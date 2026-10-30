"""The command line of the simulator."""

import argparse
import sys

from hilsim.bus import Frame, open_bus

ECHO_FRAME = Frame(0x100, bytes([0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0xA5]))
ECHO_STEP_S = 0.01
ECHO_STEPS_MAX = 100


def echo(kind: str, channel: str | None) -> int:
    """Send one frame and wait for its echo. Return the exit code for main."""
    bus = open_bus(kind, channel)
    try:
        bus.send(ECHO_FRAME)
        for _ in range(ECHO_STEPS_MAX):
            bus.step(ECHO_STEP_S)
            for frame in bus.poll():
                print(f"received 0x{frame.id:03X}: {frame.data.hex(' ')}")
                return 0
    finally:
        bus.close()
    print("no reply from the ECU", file=sys.stderr)
    return 1


def main() -> int:
    """Parse the arguments and run the command."""
    parser = argparse.ArgumentParser(prog="hilsim")
    commands = parser.add_subparsers(dest="command", required=True)
    echo_parser = commands.add_parser("echo", help="send one frame and print the reply")
    echo_parser.add_argument("--bus", choices=["sil", "hil"], default="sil")
    echo_parser.add_argument("--channel", help="the COM port of the CAN adapter (hil only)")
    args = parser.parse_args()
    if args.bus == "hil" and args.channel is None:
        parser.error("--bus hil needs --channel")
    return echo(args.bus, args.channel)


if __name__ == "__main__":
    sys.exit(main())
