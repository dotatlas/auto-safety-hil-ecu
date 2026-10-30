"""Contract tests: the DBC, the Python codec, and the committed C codec agree."""

import subprocess
import sys
from pathlib import Path

import cantools
import pytest

REPO = Path(__file__).resolve().parent.parent
DBC = REPO / "can" / "vehicle_hil.dbc"
GENERATED = REPO / "firmware" / "generated"

# firmware/tests/test_codec.c checks the same bytes and values.
VEHICLE_STATE_BYTES = bytes([0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0xA5])
VEHICLE_STATE_RAW = {
    "vehicle_speed_mps": 1790,
    "longitudinal_accel_mps2": -100,
    "steering_angle_deg": -15,
    "gear_state": 3,
    "alive_counter": 5,
    "crc8": 0xA5,
}


def test_python_codec_matches_the_c_test_vector() -> None:
    """Check that the Python codec gives the bytes that the C codec test expects."""
    message = cantools.database.load_file(DBC).get_message_by_name("VehicleState")
    assert message.encode(VEHICLE_STATE_RAW, scaling=False) == VEHICLE_STATE_BYTES
    assert (
        message.decode(VEHICLE_STATE_BYTES, scaling=False, decode_choices=False)
        == VEHICLE_STATE_RAW
    )


def test_scaling_uses_si_units() -> None:
    """Check that 17.9 m/s becomes the raw value 1790."""
    message = cantools.database.load_file(DBC).get_message_by_name("VehicleState")
    decoded = message.decode(VEHICLE_STATE_BYTES, decode_choices=False)
    assert decoded["vehicle_speed_mps"] == pytest.approx(17.9)
    assert decoded["longitudinal_accel_mps2"] == pytest.approx(-1.0)


def test_committed_codecs_are_up_to_date(tmp_path) -> None:
    """Regenerate into a temporary folder and compare with the committed files."""
    generator = REPO / "tools" / "gen_codecs.py"
    subprocess.run([sys.executable, str(generator), "--output", str(tmp_path)], check=True)
    for name in ("vehicle_hil.c", "vehicle_hil.h"):
        fresh = (tmp_path / name).read_text()
        assert fresh.splitlines() == (GENERATED / name).read_text().splitlines(), name
