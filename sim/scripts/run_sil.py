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
    "suction_authorize",
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
    12: "GROUND_FAULT",
    13: "WALL_FAILSAFE_HOLD",
    14: "HARD_FAULT",
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

FAULT_STATES = {"GROUND_FAULT", "WALL_FAILSAFE_HOLD", "HARD_FAULT"}
WALL_KINDS = {
    "up_to_wall",
    "wall_hold",
    "up_wall_down",
    "curved_wall",
    "wall_imu_stale",
    "transition_timeout",
    "wall_encoder_dropout",
    "wall_emag_loss",
    "imu_pitch_abnormal",
}


def duration_for(kind):
    if kind in {"ground", "ground_curve"}:
        return 1200
    if kind == "transition_timeout":
        return 4600
    if kind in {"up_wall_down"}:
        return 2800
    return 2600


def ramp(value0, value1, t, t0, t1):
    if t <= t0:
        return value0
    if t >= t1:
        return value1
    return int(value0 + (value1 - value0) * (t - t0) / (t1 - t0))


def wall_pitch(kind, t):
    if kind == "transition_timeout":
        if t < 260:
            return 0
        return 3000
    if kind == "up_wall_down":
        if t < 260:
            return 0
        if t < 900:
            return ramp(0, 9000, t, 260, 900)
        if t < 1500:
            return 9000
        if t < 1950:
            return ramp(9000, 0, t, 1500, 1950)
        return 0
    if kind == "imu_pitch_abnormal" and t >= 1400:
        return -3500
    if t < 260:
        return 0
    if t < 900:
        return ramp(0, 9000, t, 260, 900)
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


def generate_input_rows(kind):
    duration = duration_for(kind)
    rows = []
    for t in range(1, duration + 1):
        is_wall_kind = kind in WALL_KINDS
        manual_arm = 1 if t >= 80 else 0
        suction_authorize = 1 if is_wall_kind and t >= 260 else 0
        emag_valid = 1
        signal_quality = 520
        imu_fresh = 1
        encoder_valid = 1
        pitch_cdeg = wall_pitch(kind, t) if is_wall_kind else 0
        line_error = line_error_for(kind, t)
        left_speed = 100
        right_speed = 100
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
                "suction_authorize": suction_authorize,
                "emag_valid": emag_valid,
                "line_error": line_error,
                "signal_quality": signal_quality,
                "imu_fresh": imu_fresh,
                "pitch_cdeg": pitch_cdeg,
                "encoder_valid": encoder_valid,
                "left_count": t,
                "right_count": t,
                "left_speed_mm_s": left_speed,
                "right_speed_mm_s": right_speed,
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


def unique_state_sequence(rows):
    sequence = []
    last = None
    for row in rows:
        name = APP_STATES.get(int(row["app_state"]), f"STATE_{row['app_state']}")
        if name != last:
            sequence.append(name)
            last = name
    return sequence


def max_abs_int(rows, key):
    return max(abs(int(row[key])) for row in rows) if rows else 0


def max_int(rows, key):
    return max(int(row[key]) for row in rows) if rows else 0


def final_int(rows, key):
    return int(rows[-1][key]) if rows else 0


def assert_common(rows):
    failures = []
    if not rows:
        return ["no output rows"]
    if max_abs_int(rows, "drive_command_native") > 1000:
        failures.append("drive_command_native exceeded +/-1000")
    min_steering = min(int(row["steering_pulse_us"]) for row in rows)
    max_steering = max(int(row["steering_pulse_us"]) for row in rows)
    if min_steering < 1000 or max_steering > 2000:
        failures.append("steering_pulse_us outside 1000..2000")
    if max_int(rows, "hardware_suction_output") != 0:
        failures.append("hardware suction output became nonzero")
    return failures


def assert_expectation(expect, rows, sequence):
    failures = assert_common(rows)
    seq = set(sequence)
    final_drive = final_int(rows, "drive_command_native")
    final_steering = final_int(rows, "steering_pulse_us")

    if expect == "ground_ok":
        if "ARMED_GROUND" not in seq:
            failures.append("ARMED_GROUND not reached")
        if seq & FAULT_STATES:
            failures.append("unexpected fault state")
    elif expect == "wall_track":
        if "WALL_TRACK" not in seq:
            failures.append("WALL_TRACK not reached")
        if seq & FAULT_STATES:
            failures.append("unexpected fault state")
    elif expect == "finished":
        if "FINISHED" not in seq:
            failures.append("FINISHED not reached")
        if seq & FAULT_STATES:
            failures.append("unexpected fault state")
    elif expect == "ground_fault":
        if "GROUND_FAULT" not in seq:
            failures.append("GROUND_FAULT not reached")
        if final_drive != 0 or final_steering != 1510:
            failures.append("ground fault final outputs not safe")
    elif expect == "wall_failsafe":
        if "WALL_FAILSAFE_HOLD" not in seq:
            failures.append("WALL_FAILSAFE_HOLD not reached")
        if final_drive != 0 or final_steering != 1510:
            failures.append("wall failsafe final outputs not safe")
    elif expect == "suction_unverified":
        if "WALL_TRACK" not in seq:
            failures.append("WALL_TRACK not reached")
        if max_int(rows, "hardware_suction_output") != 0:
            failures.append("suction hardware output not locked off")
    else:
        failures.append(f"unknown expectation {expect}")
    return failures


def build_host_sil():
    script = REPO_ROOT / "sim" / "scripts" / "build_host_sil.py"
    result = subprocess.run(
        [sys.executable, str(script)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        print(result.stdout)
        return None, result.returncode
    exe = Path(result.stdout.strip().splitlines()[-1])
    return exe, 0


def run_scenario(exe, scenario):
    name = scenario["name"]
    kind = scenario["kind"]
    input_path = RESULT_DIR / f"{name}_input.csv"
    output_path = RESULT_DIR / f"{name}.csv"
    rows = generate_input_rows(kind)
    write_input_csv(input_path, rows)
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

    output_rows = read_output_csv(output_path)
    sequence = unique_state_sequence(output_rows)
    failures = assert_expectation(scenario["expect"], output_rows, sequence)
    fault_state = next((state for state in sequence if state in FAULT_STATES), "")
    surfaces = sorted({SURFACES.get(int(row["surface_state"]), row["surface_state"]) for row in output_rows})
    return {
        "name": name,
        "pass": not failures,
        "notes": "; ".join(failures) if failures else "ok; surfaces=" + "/".join(surfaces),
        "output_csv": str(output_path.relative_to(REPO_ROOT)),
        "state_sequence": sequence,
        "max_speed_cmd": max_abs_int(output_rows, "drive_command_native"),
        "max_steering_cmd": max_abs_int(output_rows, "steering_offset_us"),
        "max_logical_suction_request": max_int(output_rows, "logical_suction_request"),
        "max_hardware_suction_output": max_int(output_rows, "hardware_suction_output"),
        "fault_state": fault_state,
    }


def write_summary_files(results):
    failed = [item for item in results if not item["pass"]]
    summary = {
        "scenario_count": len(results),
        "passed": len(results) - len(failed),
        "failed": len(failed),
        "deterministic_tick_ms": 1,
        "scenarios": {item["name"]: item for item in results},
        "safety": {
            "suction_hw_verified": 0,
            "hardware_suction_output_max": max(item["max_hardware_suction_output"] for item in results),
            "real_bench_pass": False,
            "real_wall_pass": False,
        },
    }
    (RESULT_DIR / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    with (RESULT_DIR / "summary.csv").open("w", newline="", encoding="utf-8") as handle:
        fields = [
            "scenario",
            "pass",
            "actual_state_sequence",
            "max_speed_cmd",
            "max_steering_cmd",
            "max_logical_suction_request",
            "fault_state",
            "notes",
        ]
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for item in results:
            writer.writerow(
                {
                    "scenario": item["name"],
                    "pass": item["pass"],
                    "actual_state_sequence": " > ".join(item["state_sequence"]),
                    "max_speed_cmd": item["max_speed_cmd"],
                    "max_steering_cmd": item["max_steering_cmd"],
                    "max_logical_suction_request": item["max_logical_suction_request"],
                    "fault_state": item["fault_state"],
                    "notes": item["notes"],
                }
            )


def write_results_doc(results):
    lines = [
        "# Host-SIL Results",
        "",
        "These results are Host-SIL software results only. They do not verify bench hardware, suction polarity, real drive output, or wall operation.",
        "",
        "| scenario | pass | actual state sequence | max speed cmd | max steering cmd | max logical suction request | fault state | notes |",
        "| --- | ---: | --- | ---: | ---: | ---: | --- | --- |",
    ]
    for item in results:
        lines.append(
            "| {scenario} | {passed} | {sequence} | {speed} | {steering} | {suction} | {fault} | {notes} |".format(
                scenario=item["name"],
                passed="yes" if item["pass"] else "no",
                sequence=" > ".join(item["state_sequence"]),
                speed=item["max_speed_cmd"],
                steering=item["max_steering_cmd"],
                suction=item["max_logical_suction_request"],
                fault=item["fault_state"] or "-",
                notes=item["notes"].replace("|", "/"),
            )
        )
    lines.extend(
        [
            "",
            "Current status separation:",
            "",
            "- Code logic pass: measured by these Host-SIL assertions.",
            "- Host-SIL pass: measured by `sim/results/summary.json` and per-scenario CSV files.",
            "- Keil C251 build pass: measured separately by the Keil build log.",
            "- Real bench pass: not performed.",
            "- Real wall pass: not performed.",
        ]
    )
    (DOCS_DIR / "SIL_RESULTS.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_test_matrix_doc(scenarios):
    lines = [
        "# Host-SIL Test Matrix",
        "",
        "| Scenario | Input trajectory | Expected result | Assertions |",
        "| --- | --- | --- | --- |",
    ]
    descriptions = {
        "ground": "flat pitch, valid sensors, straight line",
        "ground_curve": "flat pitch, alternating left/right line error",
        "up_to_wall": "ground-to-wall pitch ramp and valid wall hold",
        "wall_hold": "ground-to-wall ramp followed by vertical hold",
        "up_wall_down": "ground-to-wall ramp, wall hold, transition-down pitch ramp",
        "curved_wall": "wall pitch with alternating line error",
        "ground_imu_stale": "ground run with IMU freshness dropped after arming",
        "wall_imu_stale": "wall run with IMU freshness dropped during wall track",
        "transition_timeout": "transition-up pitch without reaching wall threshold",
        "wall_encoder_dropout": "wall run with encoder validity dropped",
        "wall_emag_loss": "wall run with electromagnetic signal dropped",
        "imu_pitch_abnormal": "wall run with abnormal negative pitch jump",
    }
    for scenario in scenarios:
        lines.append(
            f"| {scenario['name']} | {descriptions[scenario['kind']]} | {scenario['expect']} | state sequence, fault state, drive/steering range, suction output lock |"
        )
    (DOCS_DIR / "SIL_TEST_MATRIX.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    DOCS_DIR.mkdir(parents=True, exist_ok=True)
    scenarios = json.loads(SCENARIO_FILE.read_text(encoding="utf-8"))
    write_test_matrix_doc(scenarios)

    exe, build_code = build_host_sil()
    if build_code != 0 or exe is None:
        return build_code or 2

    results = [run_scenario(exe, scenario) for scenario in scenarios]
    write_summary_files(results)
    write_results_doc(results)
    failed = [item for item in results if not item["pass"]]
    print(f"Host-SIL scenarios: {len(results) - len(failed)}/{len(results)} passed")
    if failed:
        for item in failed:
            print(f"{item['name']}: {item['notes']}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
