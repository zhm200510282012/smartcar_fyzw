import csv
import json
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCENARIO_FILE = REPO_ROOT / "sim" / "scenarios" / "scenarios.json"
RESULT_DIR = REPO_ROOT / "sim" / "results"
DOCS_DIR = REPO_ROOT / "Docs"

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
]

APP_STATES = {
    0: "BOOT",
    1: "SELF_CHECK",
    2: "SENSOR_CALIBRATION",
    3: "SAFE_GROUND_READY",
    4: "ARMED_GROUND",
    5: "PRECHARGE",
    6: "APPROACH_TRANSITION",
    7: "TRANSITION_UP",
    8: "WALL_TRACK",
    9: "TRANSITION_DOWN",
    10: "GROUND_RECOVERY",
    11: "FINISHED",
    12: "SUCTION_LOCKOUT",
    13: "GROUND_FAULT",
    14: "WALL_FAILSAFE_HOLD",
    15: "HARD_FAULT",
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
    "PRECHARGE",
    "APPROACH_TRANSITION",
    "TRANSITION_UP",
    "WALL_TRACK",
    "TRANSITION_DOWN",
    "WALL_FAILSAFE_HOLD",
}
FAULT_STATES = {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT"}
P0_PRECHARGE_SCENARIOS = {
    "S03_ground_to_wall_transition_radius",
    "S04_vertical_wall_track",
    "S05_wall_to_ground_transition",
}


def duration_for(kind):
    if kind in {"ground", "ground_curve", "suction_lockout"}:
        return 1200
    if kind == "transition_timeout":
        return 4600
    if kind == "up_wall_down":
        return 2800
    return 2600


def ramp(value0, value1, t, t0, t1):
    if t <= t0:
        return value0
    if t >= t1:
        return value1
    return int(value0 + (value1 - value0) * (t - t0) / (t1 - t0))


def wall_pitch(kind, t):
    if kind in {"ground", "ground_curve", "ground_imu_stale", "suction_lockout"}:
        return 0
    if kind == "transition_timeout":
        if t < 600:
            return 0
        return 3000
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
    if kind == "imu_pitch_abnormal" and t >= 1400:
        return -3500
    if t < 600:
        return 0
    if t < 1100:
        return ramp(0, 9000, t, 600, 1100)
    if kind == "curved_wall":
        if (t // 160) % 2 == 0:
            return 7200
        return 8400
    return 9000


def line_error_for(kind, t):
    if kind == "ground_curve":
        if t < 350:
            return -180
        if t < 700:
            return 180
        return 0
    if kind == "curved_wall":
        return -120 if (t // 120) % 2 == 0 else 120
    return 0


def candidate_for(kind, t):
    if kind in {
        "up_to_wall",
        "wall_hold",
        "up_wall_down",
        "curved_wall",
        "wall_imu_stale",
        "transition_timeout",
        "wall_encoder_dropout",
        "wall_emag_loss",
        "imu_pitch_abnormal",
        "suction_lockout",
    }:
        return 1 if t >= 220 else 0
    return 0


def generate_input_rows(kind):
    rows = []
    for t in range(1, duration_for(kind) + 1):
        candidate = candidate_for(kind, t)
        manual_arm = 1 if t >= 80 else 0
        suction_authorize = 1 if candidate and t >= 220 else 0
        emag_valid = 1
        signal_quality = 520
        imu_fresh = 1
        encoder_valid = 1
        line_error = line_error_for(kind, t)
        pitch_cdeg = wall_pitch(kind, t)
        power_ok = 1

        if kind == "ground_imu_stale" and t >= 520:
            imu_fresh = 0
        if kind == "wall_imu_stale" and t >= 1400:
            imu_fresh = 0
        if kind == "wall_encoder_dropout" and t >= 1400:
            encoder_valid = 0
        if kind == "wall_emag_loss" and t >= 1400:
            emag_valid = 0
            signal_quality = 0

        rows.append(
            {
                "time_ms": t,
                "manual_arm": manual_arm,
                "manual_suction_authorize": suction_authorize,
                "transition_candidate": candidate,
                "emag_valid": emag_valid,
                "line_error": line_error,
                "signal_quality": signal_quality,
                "imu_fresh": imu_fresh,
                "pitch_cdeg": pitch_cdeg,
                "encoder_valid": encoder_valid,
                "left_count": t,
                "right_count": t,
                "left_speed_mm_s": 100,
                "right_speed_mm_s": 100,
                "power_ok": power_ok,
            }
        )
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


def assert_common(rows):
    failures = []
    if not rows:
        return ["no output rows"]

    previous_time = 0
    previous_state = None
    previous_elapsed = 0
    for row in rows:
        time_ms = int(row["time_ms"])
        state_value = int(row["app_state"])
        surface_value = int(row["surface_state"])
        elapsed = int(row["state_elapsed_ms"])
        if state_value not in APP_STATES:
            failures.append(f"undefined app_state {state_value}")
        if surface_value not in SURFACES:
            failures.append(f"undefined surface_state {surface_value}")
        if time_ms <= previous_time:
            failures.append("time_ms is not strictly increasing")
        if previous_state == state_value and elapsed < previous_elapsed:
            failures.append("state_elapsed_ms wrapped without state change")
        previous_time = time_ms
        previous_state = state_value
        previous_elapsed = elapsed

    if max_abs_int(rows, "drive_command_native") > 1000:
        failures.append("drive_command_native exceeded +/-1000")
    if min(int(row["steering_pulse_us"]) for row in rows) < 1000:
        failures.append("steering_pulse_us below 1000")
    if max_int(rows, "steering_pulse_us") > 2000:
        failures.append("steering_pulse_us above 2000")
    if max_int(rows, "hardware_suction_output") != 0:
        failures.append("hardware suction output became nonzero")
    return failures


def drive_after_state(rows, state_name_to_find):
    seen = False
    values = []
    for row in rows:
        if state_name(row["app_state"]) == state_name_to_find:
            seen = True
        if seen:
            values.append(abs(int(row["drive_command_native"])))
    return max(values) if values else 0


def assert_expectation(scenario, rows, sequence):
    failures = assert_common(rows)
    seq = set(sequence)
    expect = scenario["expect"]
    profile = scenario["profile"]
    name = scenario["name"]

    if profile == "guard":
        if max_int(rows, "logical_suction_request") != 0:
            failures.append("guard profile produced logical suction request")
    if profile == "logical_wall":
        if max_int(rows, "logical_suction_request") <= 0:
            failures.append("logical wall profile did not produce logical suction request")

    if name in P0_PRECHARGE_SCENARIOS:
        precharge_time = first_time(rows, lambda row: state_name(row["app_state"]) == "PRECHARGE")
        observed_time = first_time(rows, lambda row: int(row["transition_up_observed"]) != 0)
        if precharge_time is None:
            failures.append("PRECHARGE not reached")
        if observed_time is None:
            failures.append("transition_up_observed never became true")
        if precharge_time is not None and observed_time is not None and precharge_time >= observed_time:
            failures.append("PRECHARGE did not occur before transition_up_observed")

    if expect == "ground_ok":
        if "ARMED_GROUND" not in seq:
            failures.append("ARMED_GROUND not reached")
        if seq & WALL_STATES:
            failures.append("ground scenario entered wall-related state")
        if any(SURFACES.get(int(row["surface_state"])) in {"TRANSITION_UP", "WALL", "TRANSITION_DOWN"} for row in rows):
            failures.append("ground scenario observed wall-related surface")
        if seq & FAULT_STATES:
            failures.append("ground scenario entered fault state")
    elif expect == "wall_track":
        for required in ("PRECHARGE", "APPROACH_TRANSITION", "TRANSITION_UP", "WALL_TRACK"):
            if required not in seq:
                failures.append(f"{required} not reached")
        if seq & FAULT_STATES:
            failures.append("unexpected fault state")
    elif expect == "finished":
        for required in ("WALL_TRACK", "TRANSITION_DOWN", "GROUND_RECOVERY", "FINISHED"):
            if required not in seq:
                failures.append(f"{required} not reached")
        if seq & FAULT_STATES:
            failures.append("unexpected fault state")
    elif expect == "ground_fault":
        if "GROUND_FAULT" not in seq:
            failures.append("GROUND_FAULT not reached")
    elif expect == "wall_failsafe":
        if "WALL_FAILSAFE_HOLD" not in seq:
            failures.append("WALL_FAILSAFE_HOLD not reached")
        if int(rows[-1]["drive_command_native"]) != 0 or int(rows[-1]["steering_pulse_us"]) != 1510:
            failures.append("wall failsafe final outputs not safe")
    elif expect == "suction_lockout":
        if "SUCTION_LOCKOUT" not in seq:
            failures.append("SUCTION_LOCKOUT not reached")
        if "WALL_TRACK" in seq:
            failures.append("S10 reached WALL_TRACK")
        illegal_wall = seq & WALL_STATES
        if illegal_wall:
            failures.append("S10 entered wall-related states: " + ",".join(sorted(illegal_wall)))
        if max_int(rows, "logical_suction_request") != 0:
            failures.append("S10 logical suction request became nonzero")
        if drive_after_state(rows, "SUCTION_LOCKOUT") != 0:
            failures.append("drive command after SUCTION_LOCKOUT was nonzero")
    else:
        failures.append(f"unknown expectation {expect}")

    return failures


def build_profile(profile):
    script = REPO_ROOT / "sim" / "scripts" / "build_host_sil.py"
    result = subprocess.run(
        [sys.executable, str(script), "--profile", profile],
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
    name = scenario["name"]
    profile = scenario["profile"]
    input_path = RESULT_DIR / f"{profile}_{name}_input.csv"
    output_path = RESULT_DIR / f"{profile}_{name}.csv"
    write_input_csv(input_path, generate_input_rows(scenario["kind"]))

    result = subprocess.run(
        [str(exe), str(input_path), str(output_path)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        return {
            "name": name,
            "profile": profile,
            "pass": False,
            "notes": result.stdout.strip(),
            "output_csv": str(output_path.relative_to(REPO_ROOT)),
            "state_sequence": [],
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
    if fault_state == "" and "SUCTION_LOCKOUT" in sequence:
        fault_state = "SUCTION_LOCKOUT"
    surfaces = sorted({SURFACES.get(int(row["surface_state"]), row["surface_state"]) for row in rows})
    return {
        "name": name,
        "profile": profile,
        "pass": not failures,
        "notes": "; ".join(failures) if failures else "ok; surfaces=" + "/".join(surfaces),
        "output_csv": str(output_path.relative_to(REPO_ROOT)),
        "state_sequence": sequence,
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


def write_summary_files(results):
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
    text = json.dumps(summary, indent=2)
    (RESULT_DIR / "p0_safety_gate_summary.json").write_text(text, encoding="utf-8")
    (RESULT_DIR / "summary.json").write_text(text, encoding="utf-8")

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
    for path in (RESULT_DIR / "p0_safety_gate_summary.csv", RESULT_DIR / "summary.csv"):
        with path.open("w", newline="", encoding="utf-8") as handle:
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


def write_results_doc(results):
    lines = [
        "# Host-SIL Results",
        "",
        "These P0 safety-gate results are Host-SIL software results only. Logical Wall Profile enables only a theoretical host-side suction request and does not verify ESC, fan, adhesion, real drive output, or wall capability.",
        "",
        "| scenario | profile | pass | actual state sequence | max speed cmd | max steering cmd | max logical suction request | max hardware suction output | fault state | notes |",
        "| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | --- | --- |",
    ]
    for item in results:
        lines.append(
            "| {scenario} | {profile} | {passed} | {sequence} | {speed} | {steering} | {logical} | {hardware} | {fault} | {notes} |".format(
                scenario=item["name"],
                profile=item["profile"],
                passed="yes" if item["pass"] else "no",
                sequence=" > ".join(item["state_sequence"]),
                speed=item["max_speed_cmd"],
                steering=item["max_steering_cmd"],
                logical=item["max_logical_suction_request"],
                hardware=item["max_hardware_suction_output"],
                fault=item["fault_state"] or "-",
                notes=item["notes"].replace("|", "/"),
            )
        )
    lines.extend(
        [
            "",
            "Status separation:",
            "",
            "- Guard Profile validates suction lockout while `SUCTION_HW_VERIFIED=0`.",
            "- Logical Wall Profile validates only host-side state-machine flow and logical suction requests.",
            "- Keil C251 build pass is measured separately by the Keil build log.",
            "- Real bench pass: not performed.",
            "- Real wall pass: not performed.",
        ]
    )
    (DOCS_DIR / "SIL_RESULTS.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_test_matrix_doc(scenarios):
    lines = [
        "# Host-SIL Test Matrix",
        "",
        "| Scenario | Profile | Input trajectory | Expected result | Assertions |",
        "| --- | --- | --- | --- | --- |",
    ]
    descriptions = {
        "ground": "flat pitch, valid sensors, no transition candidate",
        "ground_curve": "flat pitch, alternating left/right line error, no transition candidate",
        "up_to_wall": "candidate event precedes pitch ramp from ground to wall",
        "wall_hold": "candidate event, precharge dwell, pitch ramp, vertical hold",
        "up_wall_down": "candidate event, wall hold, transition-down pitch ramp",
        "curved_wall": "candidate event, wall pitch with alternating line error",
        "ground_imu_stale": "ground run with IMU freshness dropped after arming",
        "wall_imu_stale": "logical wall run with IMU freshness dropped during wall track",
        "transition_timeout": "candidate event and transition-up pitch without reaching wall threshold",
        "suction_lockout": "guard profile candidate event and suction authorization with no verified suction",
        "wall_encoder_dropout": "logical wall run with encoder validity dropped",
        "wall_emag_loss": "logical wall run with electromagnetic signal dropped",
        "imu_pitch_abnormal": "logical wall run with abnormal negative pitch jump",
    }
    for scenario in scenarios:
        lines.append(
            f"| {scenario['name']} | {scenario['profile']} | {descriptions[scenario['kind']]} | {scenario['expect']} | state sequence, profile gates, precharge ordering, command ranges, suction output lock |"
        )
    (DOCS_DIR / "SIL_TEST_MATRIX.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    DOCS_DIR.mkdir(parents=True, exist_ok=True)
    scenarios = json.loads(SCENARIO_FILE.read_text(encoding="utf-8"))
    write_test_matrix_doc(scenarios)

    executables = {}
    for profile in ("guard", "logical_wall"):
        exe, code = build_profile(profile)
        if code != 0 or exe is None:
            return code or 2
        executables[profile] = exe

    results = [run_scenario(executables[scenario["profile"]], scenario) for scenario in scenarios]
    write_summary_files(results)
    write_results_doc(results)
    failed = [item for item in results if not item["pass"]]
    print(f"P0 safety gate scenarios: {len(results) - len(failed)}/{len(results)} passed")
    if failed:
        for item in failed:
            print(f"{item['name']} ({item['profile']}): {item['notes']}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
