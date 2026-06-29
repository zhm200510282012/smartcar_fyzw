import csv
import json
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
RESULT_DIR = REPO_ROOT / "sim" / "results"

INPUT_HEADER = [
    "time_ms",
    "manual_arm",
    "manual_suction_authorize",
    "transition_candidate",
    "emag_valid",
    "line_error",
    "signal_quality",
    "imu_fresh",
    "pitch_cdeg",
    "encoder_valid",
    "left_count",
    "right_count",
    "left_speed_mm_s",
    "right_speed_mm_s",
    "power_ok",
    "imu_id_ok",
    "kill_switch",
    "control_period_ok",
    "force_app_state",
]

APP_STATES = {
    0: "BOOT",
    1: "SELF_CHECK",
    2: "SENSOR_CALIBRATION",
    3: "SAFE_GROUND_READY",
    4: "ARMED_GROUND",
    5: "GROUND_TRACK",
    6: "TRANSITION_CANDIDATE",
    7: "SUCTION_PRECHARGE",
    8: "APPROACH_TRANSITION",
    9: "TRANSITION_UP",
    10: "WALL_TRACK",
    11: "CYLINDER_TRACK",
    12: "TRANSITION_DOWN",
    13: "GROUND_RECOVERY",
    14: "SEESAW_PASS",
    15: "FINISHED",
    16: "GROUND_FAULT",
    17: "SUCTION_LOCKOUT",
    18: "WALL_FAILSAFE_HOLD",
    19: "HARD_FAULT",
}

SURFACES = {
    0: "UNKNOWN",
    1: "GROUND",
    2: "TRANSITION_UP",
    3: "WALL",
    4: "CYLINDER",
    5: "TRANSITION_DOWN",
    6: "DISABLED_UNVERIFIED",
}

WALL_STATES = {
    "TRANSITION_CANDIDATE",
    "SUCTION_PRECHARGE",
    "APPROACH_TRANSITION",
    "TRANSITION_UP",
    "WALL_TRACK",
    "CYLINDER_TRACK",
    "TRANSITION_DOWN",
    "WALL_FAILSAFE_HOLD",
}
FAULT_STATES = {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT", "SUCTION_LOCKOUT"}

SCENARIOS = [
    {"name": "S01_ground_straight", "profile": "guard", "kind": "ground", "expect": "ground_ok"},
    {"name": "S02_ground_curve_left_right", "profile": "guard", "kind": "ground_curve", "expect": "ground_ok"},
    {"name": "S03_precharge_before_transition", "profile": "logical_wall", "kind": "up_to_wall", "expect": "wall_track"},
    {"name": "S04_wall_track", "profile": "logical_wall", "kind": "wall_hold", "expect": "wall_track"},
    {"name": "S05_wall_to_ground_transition", "profile": "logical_wall", "kind": "up_wall_down", "expect": "ground_after_recovery"},
    {"name": "S06_curved_or_cylinder_mode", "profile": "logical_wall", "kind": "curved_cylinder", "expect": "cylinder_track"},
    {"name": "S07_stale_sensor_on_ground", "profile": "guard", "kind": "ground_imu_stale", "expect": "ground_fault"},
    {"name": "S08_stale_sensor_on_wall", "profile": "logical_wall", "kind": "wall_imu_stale", "expect": "wall_failsafe"},
    {"name": "S09_transition_timeout", "profile": "logical_wall", "kind": "transition_timeout", "expect": "wall_failsafe"},
    {"name": "S10_suction_unverified_lockout", "profile": "guard", "kind": "suction_lockout", "expect": "suction_lockout"},
    {"name": "S11_encoder_dropout", "profile": "logical_wall", "kind": "wall_encoder_dropout", "expect": "wall_failsafe"},
    {"name": "S12_emag_loss", "profile": "logical_wall", "kind": "wall_emag_loss", "expect": "wall_failsafe"},
    {"name": "S13_imu_abnormal", "profile": "logical_wall", "kind": "imu_invalid", "expect": "wall_failsafe"},
    {"name": "S14_unarmed_steering_centered", "profile": "guard", "kind": "unarmed_ground", "expect": "centered"},
    {"name": "S15_ground_recovery_returns_ground", "profile": "logical_wall", "kind": "up_wall_down", "expect": "ground_after_recovery"},
    {"name": "S16_illegal_state_hard_fault", "profile": "guard", "kind": "illegal_state", "expect": "hard_fault"},
    {"name": "S17_transition_down_no_rebound_up", "profile": "logical_wall", "kind": "down_rebound", "expect": "no_rebound"},
    {"name": "S18_single_frame_imu_spike_rejection", "profile": "guard", "kind": "single_imu_spike", "expect": "spike_rejected"},
    {"name": "S19_kill_overrides_all_outputs", "profile": "logical_wall", "kind": "kill_on_wall", "expect": "kill_zero"},
    {"name": "S20_control_period_overrun_fault", "profile": "guard", "kind": "control_overrun", "expect": "overrun_fault"},
]


def duration_for(kind):
    if kind in {"ground", "ground_curve", "ground_imu_stale", "unarmed_ground", "single_imu_spike", "control_overrun"}:
        return 1200
    if kind == "transition_timeout":
        return 4600
    if kind == "up_wall_down":
        return 3800
    if kind == "down_rebound":
        return 5000
    if kind == "illegal_state":
        return 600
    return 2800


def ramp(value0, value1, t, t0, t1):
    if t <= t0:
        return value0
    if t >= t1:
        return value1
    return int(value0 + (value1 - value0) * (t - t0) / (t1 - t0))


def pitch_for(kind, t):
    if kind in {"ground", "ground_curve", "ground_imu_stale", "unarmed_ground", "control_overrun"}:
        return 0
    if kind == "single_imu_spike":
        return -5000 if t == 720 else 0
    if kind == "transition_timeout":
        return 3000 if t >= 600 else 0
    if kind == "up_wall_down":
        if t < 600:
            return 0
        if t < 1100:
            return ramp(0, 9000, t, 600, 1100)
        if t < 1500:
            return 9000
        if t < 1950:
            return ramp(9000, 0, t, 1500, 1950)
        return 0
    if kind == "down_rebound":
        if t < 600:
            return 0
        if t < 1100:
            return ramp(0, 9000, t, 600, 1100)
        if t < 1450:
            return 9000
        if t < 1700:
            return ramp(9000, 0, t, 1450, 1700)
        if t < 1900:
            return 0
        return 9000
    if kind == "curved_cylinder":
        if t < 600:
            return 0
        if t < 1100:
            return ramp(0, 9000, t, 600, 1100)
        if 1450 <= t < 1750:
            return -3000
        return 9000
    if t < 600:
        return 0
    if t < 1100:
        return ramp(0, 9000, t, 600, 1100)
    return 9000


def line_error_for(kind, t):
    if kind == "ground_curve":
        if t < 350:
            return -180
        if t < 700:
            return 180
        return 0
    if kind in {"curved_cylinder", "kill_on_wall"}:
        return -120 if (t // 120) % 2 == 0 else 120
    return 0


def candidate_for(kind, t):
    if kind in {"up_wall_down", "down_rebound"}:
        return 1 if 220 <= t < 1500 else 0
    transition_kinds = {
        "up_to_wall",
        "wall_hold",
        "up_wall_down",
        "curved_cylinder",
        "wall_imu_stale",
        "transition_timeout",
        "suction_lockout",
        "wall_encoder_dropout",
        "wall_emag_loss",
        "imu_invalid",
        "down_rebound",
        "kill_on_wall",
    }
    return 1 if kind in transition_kinds and t >= 220 else 0


def generate_rows(kind):
    rows = []
    for t in range(1, duration_for(kind) + 1):
        candidate = candidate_for(kind, t)
        manual_arm = 0 if kind == "unarmed_ground" else (1 if t >= 80 else 0)
        row = {
            "time_ms": t,
            "manual_arm": manual_arm,
            "manual_suction_authorize": 1 if candidate and t >= 220 else 0,
            "transition_candidate": candidate,
            "emag_valid": 1,
            "line_error": line_error_for(kind, t),
            "signal_quality": 520,
            "imu_fresh": 1,
            "pitch_cdeg": pitch_for(kind, t),
            "encoder_valid": 1,
            "left_count": t,
            "right_count": t,
            "left_speed_mm_s": 100,
            "right_speed_mm_s": 100,
            "power_ok": 1,
            "imu_id_ok": 1,
            "kill_switch": 0,
            "control_period_ok": 1,
            "force_app_state": -1,
        }
        if kind == "ground_imu_stale" and t >= 520:
            row["imu_fresh"] = 0
        if kind == "wall_imu_stale" and t >= 1400:
            row["imu_fresh"] = 0
        if kind == "wall_encoder_dropout" and t >= 1400:
            row["encoder_valid"] = 0
        if kind == "wall_emag_loss" and t >= 1400:
            row["emag_valid"] = 0
            row["signal_quality"] = 0
        if kind == "imu_invalid" and t >= 1400:
            row["imu_id_ok"] = 0
        if kind == "illegal_state" and t >= 220:
            row["force_app_state"] = 99
        if kind == "kill_on_wall" and t >= 1500:
            row["kill_switch"] = 1
        if kind == "control_overrun" and t >= 520:
            row["control_period_ok"] = 0
        rows.append(row)
    return rows


def write_input_csv(path, rows):
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=INPUT_HEADER)
        writer.writeheader()
        writer.writerows(rows)


def read_output_csv(path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def state_name(value):
    return APP_STATES.get(int(value), f"STATE_{value}")


def unique_state_sequence(rows):
    sequence = []
    last = None
    for row in rows:
        name = state_name(row["app_state"])
        if name != last:
            sequence.append(name)
            last = name
    return sequence


def max_int(rows, key):
    return max(int(row[key]) for row in rows) if rows else 0


def max_abs_int(rows, key):
    return max(abs(int(row[key])) for row in rows) if rows else 0


def first_time(rows, predicate):
    for row in rows:
        if predicate(row):
            return int(row["time_ms"])
    return None


def failures_common(rows):
    failures = []
    if not rows:
        return ["no output rows"]
    if max_abs_int(rows, "drive_command_native") > 1000:
        failures.append("drive command exceeded +/-1000")
    if max_abs_int(rows, "left_drive_command_native") > 1000:
        failures.append("left drive command exceeded +/-1000")
    if max_abs_int(rows, "right_drive_command_native") > 1000:
        failures.append("right drive command exceeded +/-1000")
    for key in ("steering_pulse_us", "steering_left_pulse_us", "steering_right_pulse_us"):
        if min(int(row[key]) for row in rows) < 1000:
            failures.append(f"{key} below 1000")
        if max_int(rows, key) > 2000:
            failures.append(f"{key} above 2000")
    if max_int(rows, "hardware_suction_output") != 0:
        failures.append("hardware suction output became nonzero")
    return failures


def assert_centered_after(rows, state):
    failures = []
    seen = False
    for row in rows:
        if state_name(row["app_state"]) == state:
            seen = True
        if seen:
            if int(row["drive_command_native"]) != 0:
                failures.append(f"drive nonzero after {state}")
                break
            if int(row["steering_pulse_us"]) != 1510:
                failures.append(f"steering not centered after {state}")
                break
    return failures


def assert_expectation(scenario, rows, sequence):
    failures = failures_common(rows)
    seq = set(sequence)
    expect = scenario["expect"]

    if scenario["profile"] == "guard" and max_int(rows, "logical_suction_request") != 0:
        failures.append("guard profile produced logical suction request")
    if scenario["profile"] == "logical_wall" and scenario["kind"] not in {"kill_on_wall"}:
        if expect not in {"wall_failsafe", "finished_centered", "finished", "ground_after_recovery", "cylinder_track", "wall_track", "no_rebound"}:
            pass

    if scenario["name"] == "S03_precharge_before_transition":
        precharge_time = first_time(rows, lambda row: state_name(row["app_state"]) == "SUCTION_PRECHARGE")
        observed_time = first_time(rows, lambda row: int(row["transition_up_observed"]) != 0)
        if precharge_time is None or observed_time is None or precharge_time >= observed_time:
            failures.append("precharge did not precede transition-up observation")

    if expect == "ground_ok":
        if "GROUND_TRACK" not in seq:
            failures.append("GROUND_TRACK not reached")
        if seq & WALL_STATES:
            failures.append("ground scenario entered wall-related state")
        if seq & FAULT_STATES:
            failures.append("ground scenario entered fault state")
    elif expect == "wall_track":
        for required in ("TRANSITION_CANDIDATE", "SUCTION_PRECHARGE", "TRANSITION_UP", "WALL_TRACK"):
            if required not in seq:
                failures.append(f"{required} not reached")
        if seq & {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT"}:
            failures.append("unexpected fault state")
    elif expect == "finished":
        if "FINISHED" not in seq:
            failures.append("FINISHED not reached")
    elif expect == "ground_after_recovery":
        for required in ("WALL_TRACK", "TRANSITION_DOWN", "GROUND_RECOVERY"):
            if required not in seq:
                failures.append(f"{required} not reached")
        if "FINISHED" in seq:
            failures.append("FINISHED reached without FINISH route event")
        if "GROUND_RECOVERY" in sequence:
            index = sequence.index("GROUND_RECOVERY")
            if "GROUND_TRACK" not in sequence[index + 1:]:
                failures.append("GROUND_TRACK not reached after GROUND_RECOVERY")
        if seq & {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT"}:
            failures.append("unexpected fault state")
    elif expect == "cylinder_track":
        if "CYLINDER_TRACK" not in seq:
            failures.append("CYLINDER_TRACK not reached")
        if seq & {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT"}:
            failures.append("unexpected fault state")
    elif expect == "ground_fault":
        if "GROUND_FAULT" not in seq:
            failures.append("GROUND_FAULT not reached")
    elif expect == "wall_failsafe":
        if "WALL_FAILSAFE_HOLD" not in seq:
            failures.append("WALL_FAILSAFE_HOLD not reached")
        failures.extend(assert_centered_after(rows, "WALL_FAILSAFE_HOLD"))
    elif expect == "suction_lockout":
        if "SUCTION_LOCKOUT" not in seq:
            failures.append("SUCTION_LOCKOUT not reached")
        if seq & WALL_STATES:
            failures.append("suction lockout entered wall-related state")
        failures.extend(assert_centered_after(rows, "SUCTION_LOCKOUT"))
    elif expect == "centered":
        if max_abs_int(rows, "drive_command_native") != 0:
            failures.append("unarmed drive command became nonzero")
        if max_int(rows, "steering_pulse_us") != 1510 or min(int(row["steering_pulse_us"]) for row in rows) != 1510:
            failures.append("unarmed steering not centered")
    elif expect == "finished_centered":
        if "FINISHED" not in seq:
            failures.append("FINISHED not reached")
        failures.extend(assert_centered_after(rows, "FINISHED"))
    elif expect == "hard_fault":
        if "HARD_FAULT" not in seq:
            failures.append("HARD_FAULT not reached")
        failures.extend(assert_centered_after(rows, "HARD_FAULT"))
    elif expect == "no_rebound":
        if "TRANSITION_DOWN" not in sequence:
            failures.append("TRANSITION_DOWN not reached")
        else:
            index = sequence.index("TRANSITION_DOWN")
            if "TRANSITION_UP" in sequence[index + 1:]:
                failures.append("state rebounded from TRANSITION_DOWN to TRANSITION_UP")
        if "WALL_FAILSAFE_HOLD" not in seq:
            failures.append("down rebound scenario did not fail safe")
    elif expect == "spike_rejected":
        if seq & FAULT_STATES:
            failures.append("single-frame IMU spike entered fault state")
        if "GROUND_TRACK" not in seq:
            failures.append("ground tracking not maintained after spike")
    elif expect == "kill_zero":
        if "HARD_FAULT" not in seq:
            failures.append("kill did not force HARD_FAULT")
        failures.extend(assert_centered_after(rows, "HARD_FAULT"))
        kill_time = first_time(rows, lambda row: int(row["kill_switch"]) != 0)
        if kill_time is not None:
            after = [row for row in rows if int(row["time_ms"]) >= kill_time]
            if max_int(after, "logical_suction_request") != 0:
                failures.append("logical suction request nonzero after kill")
    elif expect == "overrun_fault":
        if "GROUND_FAULT" not in seq:
            failures.append("control overrun did not force GROUND_FAULT")
        failures.extend(assert_centered_after(rows, "GROUND_FAULT"))
    else:
        failures.append(f"unknown expectation {expect}")
    return failures


def build_profile(profile):
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "build_host_sil.py"), "--profile", profile],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        print(result.stdout)
        return None, result.returncode
    return Path(result.stdout.strip().splitlines()[-1]), 0


def run_scenario(exe, scenario):
    input_path = RESULT_DIR / f"full_{scenario['profile']}_{scenario['name']}_input.csv"
    output_path = RESULT_DIR / f"full_{scenario['profile']}_{scenario['name']}.csv"
    write_input_csv(input_path, generate_rows(scenario["kind"]))
    result = subprocess.run(
        [str(exe), str(input_path), str(output_path)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        return {
            "name": scenario["name"],
            "profile": scenario["profile"],
            "pass": False,
            "notes": result.stdout.strip(),
            "state_sequence": [],
            "output_csv": str(output_path.relative_to(REPO_ROOT)),
            "max_speed_cmd": 0,
            "max_steering_cmd": 0,
            "max_logical_suction_request": 0,
            "max_hardware_suction_output": 0,
            "fault_state": "",
        }
    rows = read_output_csv(output_path)
    sequence = unique_state_sequence(rows)
    failures = assert_expectation(scenario, rows, sequence)
    fault_state = next((state for state in sequence if state in FAULT_STATES), "")
    surfaces = sorted({SURFACES.get(int(row["surface_state"]), row["surface_state"]) for row in rows})
    return {
        "name": scenario["name"],
        "profile": scenario["profile"],
        "pass": not failures,
        "notes": "; ".join(failures) if failures else "ok; surfaces=" + "/".join(surfaces),
        "state_sequence": sequence,
        "output_csv": str(output_path.relative_to(REPO_ROOT)),
        "max_speed_cmd": max_abs_int(rows, "drive_command_native"),
        "max_steering_cmd": max_abs_int(rows, "steering_offset_us"),
        "max_logical_suction_request": max_int(rows, "logical_suction_request"),
        "max_hardware_suction_output": max_int(rows, "hardware_suction_output"),
        "fault_state": fault_state,
    }


def profile_summary(results, profile):
    selected = [item for item in results if item["profile"] == profile]
    return {
        "scenario_count": len(selected),
        "passed": sum(1 for item in selected if item["pass"]),
        "failed": sum(1 for item in selected if not item["pass"]),
        "max_logical_suction_request": max((item["max_logical_suction_request"] for item in selected), default=0),
        "max_hardware_suction_output": max((item["max_hardware_suction_output"] for item in selected), default=0),
    }


def write_summary(results):
    failed = [item for item in results if not item["pass"]]
    summary = {
        "scenario_count": len(results),
        "passed": len(results) - len(failed),
        "failed": len(failed),
        "deterministic_tick_ms": 1,
        "profiles": {
            "guard": profile_summary(results, "guard"),
            "logical_wall": profile_summary(results, "logical_wall"),
        },
        "scenarios": {item["name"]: item for item in results},
        "safety": {
            "suction_hw_verified": 0,
            "hardware_suction_output_max": max((item["max_hardware_suction_output"] for item in results), default=0),
            "real_bench_pass": False,
            "real_wall_pass": False,
        },
    }
    (RESULT_DIR / "full_course_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    fields = [
        "scenario",
        "profile",
        "pass",
        "actual_state_sequence",
        "max_speed_cmd",
        "max_steering_cmd",
        "max_logical_suction_request",
        "max_hardware_suction_output",
        "fault_state",
        "notes",
    ]
    with (RESULT_DIR / "full_course_summary.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for item in results:
            writer.writerow(
                {
                    "scenario": item["name"],
                    "profile": item["profile"],
                    "pass": item["pass"],
                    "actual_state_sequence": " > ".join(item["state_sequence"]),
                    "max_speed_cmd": item["max_speed_cmd"],
                    "max_steering_cmd": item["max_steering_cmd"],
                    "max_logical_suction_request": item["max_logical_suction_request"],
                    "max_hardware_suction_output": item["max_hardware_suction_output"],
                    "fault_state": item["fault_state"],
                    "notes": item["notes"],
                }
            )
    return summary


def main():
    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    executables = {}
    for profile in ("guard", "logical_wall"):
        exe, code = build_profile(profile)
        if code != 0 or exe is None:
            return code or 2
        executables[profile] = exe
    results = [run_scenario(executables[scenario["profile"]], scenario) for scenario in SCENARIOS]
    summary = write_summary(results)
    print(f"Full-course Host-SIL scenarios: {summary['passed']}/{summary['scenario_count']} passed")
    if summary["failed"] != 0:
        for item in results:
            if not item["pass"]:
                print(f"{item['name']} ({item['profile']}): {item['notes']}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
