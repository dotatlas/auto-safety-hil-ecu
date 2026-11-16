"""Contract tests: the DBC, the Python codec, and the committed C codec agree."""

import subprocess
import sys
from pathlib import Path

import cantools
import pytest

REPO = Path(__file__).resolve().parent.parent
DBC = REPO / "can" / "vehicle_hil.dbc"
GENERATED = REPO / "firmware" / "generated"

# firmware/tests/test_codec.c checks the same raw values and bytes with the C codec.
VECTORS = {
    "VehicleState": (
        dict(vehicle_speed_mps=1790, longitudinal_accel_mps2=-100, steering_angle_rad=-15),
        dict(gear_state=3, alive_counter=5, crc8=0xA5),
        "fe069cfff13f05a5",
    ),
    "WheelSpeed": (
        dict(wheel_fl_mps=895, wheel_fr_mps=900, wheel_rl_mps=4095, wheel_rr_mps=0),
        dict(alive_counter=15, crc8=0x3C),
        "7f4338ff0f000f3c",
    ),
    "ImuData": (
        dict(accel_x_mps2=-5000, accel_y_mps2=5000, yaw_rate_radps=-2000, imu_valid=1),
        dict(alive_counter=1, crc8=0x01),
        "78ec881330180101",
    ),
    "ObstacleData": (
        dict(distance_m=3500, closing_speed_mps=1790, confidence_pct=100, pedestrian_detected=1),
        dict(radar_valid=1, alive_counter=7, crc8=0x77),
        "ac0dfe06e4010777",
    ),
    "SensorHealth": (
        dict(wheel_valid=1, imu_valid=0, radar_valid=1, localization_valid=1),
        dict(sim_fault_flags=0xBEEF, alive_counter=2, crc8=0x22),
        "0defbe0000000222",
    ),
    "DriveRequest": (
        dict(requested_accel_mps2=-1000, requested_steer_rad=2000, driver_source=2),
        dict(driver_valid=1, alive_counter=9, crc8=0x99),
        "18fcd0a700000999",
    ),
    "SafetyCommand": (
        dict(brake_request_pct=100, safety_state=3, emergency_brake_active=1, fault_code=0x1234),
        dict(alive_counter=0, crc8=0x00),
        "6413341200000000",
    ),
    "EcuStatus": (
        dict(active_fault_bitmap=0x80000001, cpu_load_pct=37, anomaly_score=100),
        dict(alive_counter=14, crc8=0xEE),
        "0100008025320eee",
    ),
    "ActuatorCommand": (
        dict(commanded_accel_mps2=-800, commanded_steer_rad=-500, override_active=1),
        dict(override_reason=2, alive_counter=3, crc8=0x33),
        "e0fc0c5e00000333",
    ),
    "EcuDiagnostics": (
        dict(crc_error_count=255, counter_jump_count=1, repeat_count=2, range_error_count=3),
        dict(rx_drop_count=4, watchdog_reset_count=15, reset_reason=6, alive_counter=8, crc8=0x88),
        "ff010203046f0888",
    ),
}


@pytest.mark.parametrize("name", VECTORS)
def test_python_codec_matches_the_c_test_vectors(name: str) -> None:
    """Check that each message encodes to and decodes from the bytes the C test expects."""
    first, rest, expected = VECTORS[name]
    raw = first | rest
    message = cantools.database.load_file(DBC).get_message_by_name(name)
    assert message.encode(raw, scaling=False, strict=True).hex() == expected
    assert message.decode(bytes.fromhex(expected), scaling=False, decode_choices=False) == raw


def test_every_message_has_a_vector() -> None:
    """Check that a new message in the DBC can't skip the contract test."""
    names = {message.name for message in cantools.database.load_file(DBC).messages}
    assert names == set(VECTORS)


def test_scaling_uses_si_units() -> None:
    """Check that 17.9 m/s becomes the raw value 1790."""
    message = cantools.database.load_file(DBC).get_message_by_name("VehicleState")
    decoded = message.decode(bytes.fromhex(VECTORS["VehicleState"][2]), decode_choices=False)
    assert decoded["vehicle_speed_mps"] == pytest.approx(17.9)
    assert decoded["longitudinal_accel_mps2"] == pytest.approx(-1.0)
    assert decoded["steering_angle_rad"] == pytest.approx(-0.015)


def test_committed_codecs_are_up_to_date(tmp_path) -> None:
    """Regenerate into a temporary folder and compare with the committed files."""
    generator = REPO / "tools" / "gen_codecs.py"
    subprocess.run([sys.executable, str(generator), "--output", str(tmp_path)], check=True)
    for name in ("vehicle_hil.c", "vehicle_hil.h"):
        fresh = (tmp_path / name).read_text()
        assert fresh.splitlines() == (GENERATED / name).read_text().splitlines(), name
