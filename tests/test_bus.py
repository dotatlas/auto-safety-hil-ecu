"""Tests for both bus backends: the echo through the C core, and the python-can wrapper."""

import can
import pytest

from hilsim.__main__ import ECHO_FRAME, ECHO_STEP_S, ECHO_STEPS_MAX, main
from hilsim.bus import Frame, HilBus, SilBus, open_bus


@pytest.mark.sil
def test_sil_echo_adds_the_offset_and_keeps_the_data() -> None:
    """Check that one frame comes back from the C core after one step."""
    bus = SilBus()
    bus.send(Frame(0x100, b"\x01\x02\x03"))
    assert bus.poll() == []
    bus.step(0.01)
    assert bus.poll() == [Frame(0x200, b"\x01\x02\x03")]
    assert bus.poll() == []


@pytest.mark.sil
def test_sil_rejects_a_missing_library(tmp_path) -> None:
    """Check that a missing build gives a clear message."""
    with pytest.raises(FileNotFoundError, match="Build the host project"):
        SilBus(tmp_path / "missing.dll")


@pytest.mark.sil
def test_echo_command_prints_the_reply(capsys, monkeypatch) -> None:
    """Check the command line against the C core."""
    monkeypatch.setattr("sys.argv", ["hilsim", "echo", "--bus", "sil"])
    assert main() == 0
    assert "received 0x200: fe 06 9c ff f1 3f 05 a5" in capsys.readouterr().out


def test_hil_bus_sends_and_receives_on_a_virtual_bus() -> None:
    """Check the python-can wrapper with the virtual interface of python-can."""
    peer = can.Bus(interface="virtual", channel="test_hil_bus")
    bus = HilBus(can.Bus(interface="virtual", channel="test_hil_bus"))
    try:
        bus.send(Frame(0x100, b"\x01\x02"))
        message = peer.recv(1.0)
        assert (message.arbitration_id, bytes(message.data)) == (0x100, b"\x01\x02")
        assert message.is_extended_id is False

        peer.send(can.Message(arbitration_id=0x200, data=b"\x03", is_extended_id=False))
        assert bus.poll() == [Frame(0x200, b"\x03")]
    finally:
        bus.close()
        peer.shutdown()


def test_open_bus_rejects_bad_arguments() -> None:
    """Check that a wrong bus name or a missing COM port gives a clear message."""
    with pytest.raises(ValueError, match="needs a channel"):
        open_bus("hil")
    with pytest.raises(ValueError, match="Unknown bus"):
        open_bus("usb")


@pytest.mark.hil
def test_hil_echo(channel: str) -> None:
    """Check the echo through the adapter and the real board."""
    bus = open_bus("hil", channel)
    try:
        bus.send(ECHO_FRAME)
        for _ in range(ECHO_STEPS_MAX):
            bus.step(ECHO_STEP_S)
            if frames := bus.poll():
                assert frames[0] == Frame(0x200, ECHO_FRAME.data)
                return
        pytest.fail("no reply from the ECU")
    finally:
        bus.close()
