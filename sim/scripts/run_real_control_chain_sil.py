import csv
import json
import os
import subprocess
import sys
from pathlib import Path

import build_host_sil


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / "build"
RESULT_DIR = REPO_ROOT / "sim" / "results"

SOURCES = [
    "sim/host/real_control_chain_runner.c",
    "sim/host/host_bsp.c",
    "App/app_scheduler.c",
    "App/app_output_arbitration.c",
    "App/app_state_machine.c",
    "App/app_safety.c",
    "App/app_telemetry.c",
    "App/app_params.c",
    "BSP/bsp_fan_esc.c",
    "Control/ctrl_adhesion.c",
    "Control/ctrl_attitude.c",
    "Control/ctrl_differential_drive.c",
    "Control/ctrl_line.c",
    "Control/ctrl_profile.c",
    "Control/ctrl_signal.c",
    "Control/ctrl_fuzzy_pid.c",
    "Control/ctrl_fuzzy_turn.c",
    "Control/ctrl_fuzzy_steering.c",
    "Control/ctrl_speed.c",
    "Control/ctrl_steering.c",
    "Control/ctrl_transition.c",
    "Control/ctrl_vehicle.c",
    "Track/track_features.c",
    "Track/track_full_course_profile.c",
    "Track/track_route_event.c",
    "Track/track_route_profile.c",
    "Track/track_state_machine.c",
    "Track/track_strategy.c",
    "Track/track_surface_state.c",
    "Track/track_wall_logic.c",
]


def exe_path():
    suffix = ".exe" if os.name == "nt" else ""
    return REPO_ROOT / "sim" / "host" / f"real_control_chain_runner{suffix}"


def build_runner():
    compiler = build_host_sil.find_compiler()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    log_path = BUILD_DIR / "real_control_chain_runner_build.log"
    if compiler is None:
        log_path.write_text(
            "No host C compiler found. Tried HOST_SIL_CC, cl, gcc, clang, cc, and known existing local compiler paths.\n",
            encoding="utf-8",
        )
        return None, 2

    output = exe_path()
    source_paths = [str(REPO_ROOT / source) for source in SOURCES]
    if build_host_sil.compiler_kind(compiler) == "msvc":
        cmd = (
            compiler
            + [
                "/nologo",
                "/W4",
                "/WX",
                "/DHOST_SIL=1",
                "/DHOST_SIL_GUARD_PROFILE=1",
                "/I.",
                "/IApp",
                "/IBSP",
                "/IControl",
                "/ITrack",
                "/Isim/host",
            ]
            + source_paths
            + ["/Fe:" + str(output)]
        )
    else:
        cmd = (
            compiler
            + [
                "-std=c99",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-DHOST_SIL=1",
                "-DHOST_SIL_GUARD_PROFILE=1",
                "-I.",
                "-IApp",
                "-IBSP",
                "-IControl",
                "-ITrack",
                "-Isim/host",
                "-o",
                str(output),
            ]
            + source_paths
        )

    result = subprocess.run(
        cmd,
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    log_path.write_text(
        "COMMAND:\n"
        + " ".join(cmd)
        + "\n\nOUTPUT:\n"
        + result.stdout
        + f"\nEXIT_CODE: {result.returncode}\n",
        encoding="utf-8",
    )
    if result.returncode != 0:
        return None, result.returncode
    return output, 0


def read_raw_results(path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def write_summary(rows):
    failed = [row for row in rows if row["pass"].lower() != "true"]
    scenarios = {}
    for row in rows:
        scenarios[row["scenario"]] = {
            "pass": row["pass"].lower() == "true",
            "metric_a": int(row["metric_a"]),
            "metric_b": int(row["metric_b"]),
            "metric_c": int(row["metric_c"]),
            "notes": row["notes"],
        }
    summary = {
        "scenario_count": len(rows),
        "passed": len(rows) - len(failed),
        "failed": len(failed),
        "scenarios": scenarios,
        "safety": {
            "suction_hw_verified": 0,
            "hardware_suction_output_max": 0,
            "real_bench_pass": False,
            "real_wall_pass": False,
        },
    }
    (RESULT_DIR / "real_control_chain_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    with (RESULT_DIR / "real_control_chain_summary.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["scenario", "pass", "metric_a", "metric_b", "metric_c", "notes"],
        )
        writer.writeheader()
        writer.writerows(rows)
    return summary


def main():
    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    runner, code = build_runner()
    if code != 0 or runner is None:
        return code or 2

    raw_csv = RESULT_DIR / "real_control_chain_raw.csv"
    result = subprocess.run(
        [str(runner), str(raw_csv)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
    rows = read_raw_results(raw_csv)
    summary = write_summary(rows)
    print(f"Real control chain Host-SIL scenarios: {summary['passed']}/{summary['scenario_count']} passed")
    if summary["failed"] != 0:
        for scenario, item in summary["scenarios"].items():
            if not item["pass"]:
                print(f"{scenario}: {item['notes']}")
        return 1
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
