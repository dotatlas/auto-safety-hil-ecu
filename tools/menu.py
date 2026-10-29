"""An interactive menu for the build, test and lint commands. run.bat starts it."""

import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CHANNEL = "{channel}"
OPTIONAL_TOOLS = {"clang-format": "scoop install llvm", "cppcheck": "scoop install cppcheck"}
STM32_BUNDLES = ("gnu-tools-for-stm32", "programmer")

Command = tuple[str, ...]

HOST_BUILD: tuple[Command, ...] = (
    ("cmake", "-S", "firmware/tests", "-B", "build/host", "-G", "Ninja"),
    ("cmake", "--build", "build/host"),
)
C_TESTS: Command = ("ctest", "--test-dir", "build/host", "--output-on-failure")
PYTHON_TESTS: Command = ("uv", "run", "pytest", "-m", "not hil")
PYTHON_LINT: tuple[Command, ...] = (
    ("uv", "run", "ruff", "check", "."),
    ("uv", "run", "ruff", "format", "--check", "."),
)
C_LINT: tuple[Command, ...] = (
    (
        "cppcheck",
        "--enable=warning,style,performance",
        "--error-exitcode=1",
        *(d for d in ("firmware/app", "firmware/rtos") if (REPO / d).is_dir()),
    ),
    (
        "clang-format",
        "--dry-run",
        "-Werror",
        *sorted(p.relative_to(REPO).as_posix() for p in REPO.glob("firmware/[ar]*/*.[ch]")),
    ),
)
FIRMWARE_BUILD: tuple[Command, ...] = (
    ("cmake", "--preset", "Debug", "-S", "firmware"),
    ("cmake", "--build", "firmware/build/Debug"),
)
FLASH: Command = (
    "STM32_Programmer_CLI",
    "-c",
    "port=SWD",
    "-w",
    "firmware/build/Debug/firmware.elf",
    "-v",
    "-rst",
)


@dataclass(frozen=True)
class Action:
    """One menu entry: its name, what it does, and the commands it runs in order."""

    name: str
    about: str
    commands: tuple[Command, ...]


ACTIONS = (
    Action(
        "Check everything",
        "All tests, linters and the firmware build. Run this before a commit.",
        (*HOST_BUILD, C_TESTS, PYTHON_TESTS, *PYTHON_LINT, *C_LINT, *FIRMWARE_BUILD),
    ),
    Action(
        "C tests",
        "Build the ECU core on the PC and run its Unity tests.",
        (*HOST_BUILD, C_TESTS),
    ),
    Action(
        "Python tests",
        "Build the SIL library and run pytest. This needs no hardware.",
        (*HOST_BUILD, PYTHON_TESTS),
    ),
    Action(
        "Echo demo",
        "Send one CAN frame through the C core on the PC and print the reply.",
        (*HOST_BUILD, ("uv", "run", "python", "-m", "hilsim", "echo", "--bus", "sil")),
    ),
    Action(
        "Python lint",
        "Check the Python code with ruff. This changes no files.",
        PYTHON_LINT,
    ),
    Action(
        "C lint",
        "Check the C code with cppcheck and clang-format. This changes no files.",
        C_LINT,
    ),
    Action(
        "Firmware build",
        "Build firmware.elf for the STM32. This does not flash it.",
        FIRMWARE_BUILD,
    ),
    Action(
        "Regenerate codecs",
        "Make the C code again from can/vehicle_hil.dbc. Run this after you edit the DBC.",
        (("uv", "run", "python", "tools/gen_codecs.py"),),
    ),
    Action(
        "Set up",
        "Get the submodules and the Python packages. Run this first on a new clone.",
        (("git", "submodule", "update", "--init"), ("uv", "sync")),
    ),
    Action(
        "Flash the board [needs hardware]",
        "Build the firmware and copy it to the board through the ST-LINK.",
        (*FIRMWARE_BUILD, FLASH),
    ),
    Action(
        "Bench tests [needs hardware]",
        "Run the HIL tests over real CAN. This asks for the COM port of the adapter.",
        (("uv", "run", "pytest", "-m", "hil", "--channel", CHANNEL),),
    ),
)


def add_stm32_tools_to_path() -> None:
    """Add the STM32Cube compiler and flasher to PATH for the commands that run_commands starts."""
    bundles = Path(os.environ.get("LOCALAPPDATA", "")) / "stm32cube" / "bundles"
    # shortcut: this sorts versions as text, keep the sort correct if two versions are installed.
    folders = [
        str(folder)
        for name in STM32_BUNDLES
        for folder in sorted((bundles / name).glob("*/bin"), reverse=True)
    ]
    os.environ["PATH"] = os.pathsep.join([*folders, os.environ.get("PATH", "")])


def run_commands(commands: tuple[Command, ...]) -> bool:
    """Run the commands in order for run_action. Stop at the first failure and return False."""
    for command in commands:
        print(f"\n> {subprocess.list2cmdline(command)}", flush=True)
        if shutil.which(command[0]) is None:
            if command[0] in OPTIONAL_TOOLS:
                print(f"SKIPPED: {command[0]} is not installed ({OPTIONAL_TOOLS[command[0]]}).")
                continue
            print(f"FAILED: {command[0]} is not installed or not on PATH.")
            return False
        if subprocess.run(command, cwd=REPO).returncode != 0:
            return False
    return True


def run_action(action: Action) -> bool:
    """Ask for any COM port, call run_commands and print the result for main."""
    print(f"\n=== {action.name}\n{action.about}")
    commands = action.commands
    if any(CHANNEL in command for command in commands):
        channel = input("COM port of the CAN adapter (for example COM5): ").strip()
        if not channel:
            print("\nFAILED: no COM port given.")
            return False
        commands = tuple(
            tuple(channel if part == CHANNEL else part for part in command) for command in commands
        )
    passed = run_commands(commands)
    skipped = [c[0] for c in commands if c[0] in OPTIONAL_TOOLS and shutil.which(c[0]) is None]
    not_run = f" (not run: {', '.join(skipped)})" if skipped else ""
    print(f"\n{'PASSED' if passed else 'FAILED'}: {action.name}{not_run}")
    return passed


def print_menu() -> None:
    """Print the numbered entries for main."""
    print("\nauto-safety-hil-ecu\n")
    for number, action in enumerate(ACTIONS, start=1):
        print(f"{number:>3}. {action.name}\n     {action.about}")
    print("  q. Quit\n")


def main() -> int:
    """Show the menu and run the chosen entries until the user quits."""
    add_stm32_tools_to_path()
    try:
        while True:
            print_menu()
            choice = input("Choose a number: ").strip().lower()
            if choice == "q":
                return 0
            if not choice.isdecimal() or not 1 <= int(choice) <= len(ACTIONS):
                print(f"\n'{choice}' is not on the menu.")
                continue
            run_action(ACTIONS[int(choice) - 1])
            input("\nPress Enter to go back to the menu.")
    except EOFError, KeyboardInterrupt:
        print()
        return 0


if __name__ == "__main__":
    sys.exit(main())
