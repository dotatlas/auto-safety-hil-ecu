"""Shared pytest options for the bench tests."""

import pytest


def pytest_addoption(parser: pytest.Parser) -> None:
    """Add the --channel option that the hil tests read."""
    parser.addoption("--channel", help="the COM port of the CAN adapter")


@pytest.fixture
def channel(request: pytest.FixtureRequest) -> str:
    """Return the COM port for a hil test. Fail if the option is missing."""
    value = request.config.getoption("--channel")
    if value is None:
        pytest.fail("hil tests need --channel, for example --channel COM5")
    return value
