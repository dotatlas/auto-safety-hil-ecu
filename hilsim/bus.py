"""One bus interface with two backends: the host-built ECU core (SIL) and a CAN adapter (HIL)."""

import ctypes
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

import can

REPO = Path(__file__).resolve().parent.parent
CORE_LIBRARY = (
    REPO / "build" / "host" / ("ecu_core.dll" if sys.platform == "win32" else "ecu_core.so")
)
BITRATE = 500_000
ECHO_ID_OFFSET = 0x100
CORE_TX_MAX = 4
FRAME_DATA_MAX = 8


@dataclass(frozen=True)
class Frame:
    """One classic CAN frame with an 11-bit ID."""

    id: int
    data: bytes


class Bus(Protocol):
    """The calls that the simulator uses on either backend."""

    def send(self, frame: Frame) -> None:
        """Send one frame to the ECU."""

    def poll(self) -> list[Frame]:
        """Return the frames that the ECU sent since the last call."""

    def step(self, dt: float) -> None:
        """Advance the time by dt seconds."""

    def close(self) -> None:
        """Release the backend."""


class _CoreFrame(ctypes.Structure):
    """The ctypes copy of core_frame_t in core.h."""

    _fields_ = [
        ("id", ctypes.c_uint16),
        ("dlc", ctypes.c_uint8),
        ("data", ctypes.c_uint8 * FRAME_DATA_MAX),
    ]


class _CoreConfig(ctypes.Structure):
    """The ctypes copy of core_config_t in core.h."""

    _fields_ = [("echo_id_offset", ctypes.c_uint16)]


class _CoreOutput(ctypes.Structure):
    """The ctypes copy of core_output_t in core.h."""

    _fields_ = [("tx", _CoreFrame * CORE_TX_MAX), ("tx_count", ctypes.c_uint8)]


class SilBus:
    """Run the ECU core in this process, in lockstep with the caller."""

    def __init__(self, library: Path = CORE_LIBRARY) -> None:
        """Load the core library and call core_init."""
        if not library.exists():
            raise FileNotFoundError(f"{library} is missing. Build the host project first.")
        self._core = ctypes.CDLL(str(library))
        self._core.core_on_frame.argtypes = [
            ctypes.c_uint32,
            ctypes.c_uint16,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_uint8,
        ]
        self._core.core_step.argtypes = [ctypes.c_uint32, ctypes.POINTER(_CoreOutput)]
        self._core.core_init.argtypes = [ctypes.POINTER(_CoreConfig)]
        self._core.core_init(ctypes.byref(_CoreConfig(ECHO_ID_OFFSET)))
        self._now_us = 0
        self._received: list[Frame] = []

    def send(self, frame: Frame) -> None:
        """Give one frame to core_on_frame with the current time."""
        data = (ctypes.c_uint8 * FRAME_DATA_MAX)(*frame.data)
        self._core.core_on_frame(self._now_us, frame.id, data, len(frame.data))

    def poll(self) -> list[Frame]:
        """Return the frames that core_step gave since the last call."""
        received, self._received = self._received, []
        return received

    def step(self, dt: float) -> None:
        """Advance the time by dt seconds, call core_step and keep its frames for poll."""
        self._now_us += round(dt * 1_000_000)
        output = _CoreOutput()
        self._core.core_step(self._now_us // 1000, ctypes.byref(output))
        for frame in output.tx[: output.tx_count]:
            self._received.append(Frame(frame.id, bytes(frame.data[: frame.dlc])))

    def close(self) -> None:
        """Do nothing. The library stays loaded until the process ends."""


class HilBus:
    """Talk to the real ECU through a python-can bus."""

    def __init__(self, bus: can.BusABC) -> None:
        """Keep the bus. open_bus makes the bus for the real adapter."""
        self._bus = bus

    def send(self, frame: Frame) -> None:
        """Send one standard-ID frame on the bus."""
        message = can.Message(arbitration_id=frame.id, data=frame.data, is_extended_id=False)
        self._bus.send(message)

    def poll(self) -> list[Frame]:
        """Return all the frames that wait in the receive buffer. This call does not block."""
        received = []
        while (message := self._bus.recv(0)) is not None:
            received.append(Frame(message.arbitration_id, bytes(message.data)))
        return received

    def step(self, dt: float) -> None:
        """Wait for dt seconds. The real ECU runs on its own clock."""
        time.sleep(dt)

    def close(self) -> None:
        """Close the python-can bus."""
        self._bus.shutdown()


def open_bus(kind: str, channel: str | None = None) -> Bus:
    """Make the backend for the CLI and the tests. The HIL backend needs a COM port."""
    if kind == "sil":
        return SilBus()
    if kind == "hil":
        if channel is None:
            raise ValueError("The hil bus needs a channel, for example COM5.")
        return HilBus(can.Bus(interface="seeedstudio", channel=channel, bitrate=BITRATE))
    raise ValueError(f"Unknown bus {kind!r}. Use sil or hil.")
