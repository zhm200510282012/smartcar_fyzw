import csv
import json
import os
import re
import subprocess
import sys
from pathlib import Path

import build_host_sil


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / "build"
RESULT_DIR = REPO_ROOT / "sim" / "results"

SOURCES = [
    "sim/host/real_runtime_gap_runner.c",
    "App/app_state_machine.c",
    "BSP/bsp_power.c",
    "BSP/bsp_ui.c",
    "BSP/bsp_emag.c",
    "BSP/bsp_imu.c",
    "BSP/bsp_encoder.c",
    "BSP/bsp_fan_esc.c",
    "BSP/bsp_timebase.c",
    "Control/ctrl_adhesion.c",
    "Control/ctrl_line.c",
    "Track/track_route_event.c",
    "Track/track_wall_logic.c",
    "Drivers/Board/lsm6dsr_driver.c",
]

SERVO_FORBIDDEN_FILES = [
    "App/app_scheduler.c",
    "App/app_output_arbitration.c",
    "Track/track_wall_logic.c",
]


def exe_path():
    suffix = ".exe" if os.name == "nt" else ""
    return REPO_ROOT / "sim" / "host" / f"real_runtime_gap_runner{suffix}"


def build_runner():
    compiler = build_host_sil.find_compiler()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    log_path = BUILD_DIR / "real_runtime_gap_runner_build.log"
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
                "/I.",
                "/IApp",
                "/IBSP",
                "/IControl",
                "/ITrack",
                "/IDrivers/Board",
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
                "-I.",
                "-IApp",
                "-IBSP",
                "-IControl",
                "-ITrack",
                "-IDrivers/Board",
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


def no_servo_runtime_refs():
    pattern = re.compile(r"\bbsp_steering_|ctrl_steering_|ctrl_vehicle_|MotorServo|STEERING_")
    hits = []
    for item in SERVO_FORBIDDEN_FILES:
        text = (REPO_ROOT / item).read_text(encoding="utf-8", errors="replace")
        for index, line in enumerate(text.splitlines(), 1):
            if pattern.search(line):
                hits.append(f"{item}:{index}:{line.strip()}")
    return hits


def append_runtime15(rows):
    hits = no_servo_runtime_refs()
    rows.append(
        {
            "scenario": "RUNTIME15",
            "pass": "true" if not hits else "false",
            "metric_a": "0" if not hits else str(len(hits)),
            "metric_b": "0",
            "metric_c": "0",
            "notes": "C251 runtime no servo calls" if not hits else "servo refs: " + "; ".join(hits[:3]),
        }
    )


def write_summary(rows):
    failed = [row for row in rows if row["pass"].lower() != "true"]
    scenarios = {
        row["scenario"]: {
            "pass": row["pass"].lower() == "true",
            "metric_a": int(row["metric_a"]),
            "metric_b": int(row["metric_b"]),
            "metric_c": int(row["metric_c"]),
            "notes": row["notes"],
        }
        for row in rows
    }
    summary = {
        "scenario_count": len(rows),
        "passed": len(rows) - len(failed),
        "failed": len(failed),
        "scenarios": scenarios,
        "safety": {
            "fan_esc_physical_output_enable": 0,
            "wall_run_enable": 0,
            "suction_hw_verified": 0,
            "route_progress_script_enable": 0,
            "real_bench_pass": False,
            "real_wall_pass": False,
        },
    }
    (RESULT_DIR / "real_runtime_gap_closure_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    with (RESULT_DIR / "real_runtime_gap_closure_summary.csv").open("w", newline="", encoding="utf-8") as handle:
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
        print((BUILD_DIR / "real_runtime_gap_runner_build.log").read_text(encoding="utf-8", errors="replace"))
        return code or 2

    raw_csv = RESULT_DIR / "real_runtime_gap_closure_raw.csv"
    result = subprocess.run(
        [str(runner), str(raw_csv)],
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    rows = read_raw_results(raw_csv)
    append_runtime15(rows)
    summary = write_summary(rows)
    print(f"Real runtime gap Host-SIL scenarios: {summary['passed']}/{summary['scenario_count']} passed")
    if result.returncode != 0 or summary["failed"] != 0:
        sys.stdout.write(result.stdout)
        for scenario, item in summary["scenarios"].items():
            if not item["pass"]:
                print(f"{scenario}: {item['notes']}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
