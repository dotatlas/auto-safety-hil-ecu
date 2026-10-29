"""Tests for the command menu, with real child processes and no fakes."""

import sys

import menu

PASS = (sys.executable, "-c", "pass")
FAIL = (sys.executable, "-c", "raise SystemExit(1)")


def test_run_commands_stops_at_the_first_failure(tmp_path) -> None:
    """Check that a command after a failure does not run."""
    marker = tmp_path / "ran"
    touch = (sys.executable, "-c", f"open({str(marker)!r}, 'w')")
    assert menu.run_commands((PASS, FAIL, touch)) is False
    assert not marker.exists()
    assert menu.run_commands((PASS, touch)) is True
    assert marker.exists()


def test_required_tool_that_is_missing_fails(capsys) -> None:
    """Check that a missing required tool is a failure with a clear message."""
    assert menu.run_commands((("no-such-tool-here",),)) is False
    assert "no-such-tool-here is not installed" in capsys.readouterr().out


def test_optional_tool_that_is_missing_is_reported_and_skipped(capsys, monkeypatch) -> None:
    """Check that a missing linter does not stop the other commands and is not hidden."""
    monkeypatch.setitem(menu.OPTIONAL_TOOLS, "no-such-linter", "install it")
    assert menu.run_commands((("no-such-linter",), PASS)) is True
    assert "SKIPPED: no-such-linter" in capsys.readouterr().out


def test_action_result_names_the_tools_that_did_not_run(capsys, monkeypatch) -> None:
    """Check that a result with skipped linters does not look like a full pass."""
    monkeypatch.setitem(menu.OPTIONAL_TOOLS, "no-such-linter", "install it")
    assert menu.run_action(menu.Action("Lint", "about", (("no-such-linter",),))) is True
    assert "PASSED: Lint (not run: no-such-linter)" in capsys.readouterr().out


def test_action_replaces_the_channel_and_rejects_an_empty_one(monkeypatch, tmp_path) -> None:
    """Check that the typed COM port reaches the command, and that an empty one stops the action."""
    output = tmp_path / "channel"
    write = (sys.executable, "-c", "import sys; open(sys.argv[1], 'w').write(sys.argv[2])")
    action = menu.Action("Bench", "about", ((*write, str(output), menu.CHANNEL),))

    monkeypatch.setattr("builtins.input", lambda _prompt: " COM5 ")
    assert menu.run_action(action) is True
    assert output.read_text() == "COM5"

    monkeypatch.setattr("builtins.input", lambda _prompt: "")
    assert menu.run_action(action) is False


def test_menu_rejects_bad_choices_and_quits(capsys, monkeypatch) -> None:
    """Check the main loop with wrong entries, then q."""
    answers = iter(["99", "0", "abc", "q"])
    monkeypatch.setattr("builtins.input", lambda _prompt: next(answers))
    assert menu.main() == 0
    assert "'abc' is not on the menu." in capsys.readouterr().out


def test_menu_runs_the_chosen_action_and_survives_a_failure(capsys, monkeypatch) -> None:
    """Check that a failed action prints FAILED and returns to the menu."""
    monkeypatch.setattr(menu, "ACTIONS", (menu.Action("Bad", "about", (FAIL,)),))
    answers = iter(["1", "", "q"])
    monkeypatch.setattr("builtins.input", lambda _prompt: next(answers))
    assert menu.main() == 0
    assert "FAILED: Bad" in capsys.readouterr().out


def test_menu_exits_cleanly_when_input_ends(monkeypatch) -> None:
    """Check that a closed terminal ends the menu with no traceback."""

    def closed(_prompt: str) -> str:
        """Raise the error that input raises at the end of the input."""
        raise EOFError

    monkeypatch.setattr("builtins.input", closed)
    assert menu.main() == 0
