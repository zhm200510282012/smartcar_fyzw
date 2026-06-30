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
    "sim/host/timing_element_fan_runner.c",
    "sim/host/host_bsp.c",
    "App/app_scheduler.c",
    "App/app_control_tick.c",
    "App/app_output_arbitration.c",
    "App/app_state_machine.c",
    "App/app_safety.c",
    "App/app_telemetry.c",
    "App/app_params.c",
    "BSP/bsp_control_timers.c",
    "BSP/bsp_emag.c",
    "BSP/bsp_fan_esc.c",
    "BSP/bsp_timing_scope.c",
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
    return REPO_ROOT / "sim" / "host" / f"timing_element_fan_runner{suffix}"


def build_runner():
    compiler = build_host_sil.find_compiler()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    log_path = BUILD_DIR / "timing_element_fan_runner_build.log"
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
                "/DHOST_SIL_LOGICAL_WALL_PROFILE=1",
                "/DHOST_SIL_REAL_EMAG_SCAN=1",
                "/DHOST_SIL_USE_REAL_EMAG_BSP=1",
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
                "-DHOST_SIL_LOGICAL_WALL_PROFILE=1",
                "-DHOST_SIL_REAL_EMAG_SCAN=1",
                "-DHOST_SIL_USE_REAL_EMAG_BSP=1",
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
    log_path.write_text(result.stdout, encoding="utf-8")
    return output, result.returncode


def summarize(raw_csv):
    rows = list(csv.DictReader(raw_csv.read_text(encoding="utf-8").splitlines()))
    scenarios = {
        row["scenario"]: {
            "pass": row["status"] == "PASS",
            "detail": row["detail"],
            "values": [row["value1"], row["value2"], row["value3"]],
        }
        for row in rows
    }
    passed = sum(1 for row in rows if row["status"] == "PASS")
    summary = {
        "scenario_count": len(rows),
        "passed": passed,
        "failed": len(rows) - passed,
        "scenarios": scenarios,
        "safety": {
            "fan_esc_physical_output_enable": 0,
            "wall_run_enable": 0,
            "suction_hw_verified": 0,
            "fan_bench_test_enable": 0,
            "real_wall_pass": False,
            "real_bench_pass": False,
        },
    }
    return summary


def main():
    output, code = build_runner()
    if code != 0 or output is None:
        return code

    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    raw_csv = RESULT_DIR / "timing_element_fan_raw.csv"
    result = subprocess.run(
        [str(output), str(raw_csv)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    (BUILD_DIR / "timing_element_fan_runner.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        return result.returncode

    summary = summarize(raw_csv)
    summary_path = RESULT_DIR / "timing_element_fan_summary.json"
    csv_path = RESULT_DIR / "timing_element_fan_summary.csv"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True), encoding="utf-8")
    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["scenario", "pass", "detail"])
        for name, data in sorted(summary["scenarios"].items()):
            writer.writerow([name, int(data["pass"]), data["detail"]])
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0 if summary["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
