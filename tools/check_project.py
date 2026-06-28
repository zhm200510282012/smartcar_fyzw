#!/usr/bin/env python3
"""Static integrity checks for the FYZW AI8051U project skeleton.

This script intentionally avoids proving real hardware correctness. It checks
that the generated project is conservative when hardware evidence is missing.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


REQUIRED_FILES = [
    "AUDIT_INPUTS.md",
    "Docs/RULES_TRACEABILITY.md",
    "Docs/HW_RESOURCE_MAP.md",
    "Docs/NEGATIVE_PRESSURE_HW_AUDIT.md",
    "Docs/LEGACY_MIGRATION_AUDIT.md",
    "Docs/HW_CONFLICTS.md",
    "Docs/ASSUMPTIONS_AND_LIMITS.md",
    "Docs/ADHESION_CONTROL_DESIGN.md",
    "Docs/WALL_CLIMB_FMEA.md",
    "Docs/BUILD_REPORT.md",
    "Docs/TEST_CHECKLIST.md",
    "Docs/NEGATIVE_PRESSURE_TEST_PLAN.md",
    "Docs/TUNING_GUIDE.md",
    "Docs/KNOWN_LIMITATIONS.md",
    "Docs/CHANGELOG.md",
    "MDK_Project/smartcar_fyzw.uvproj",
    "App/main.c",
    "App/app_config.h",
    "App/app_types.h",
    "App/app_scheduler.c",
    "App/app_scheduler.h",
    "App/app_state_machine.c",
    "App/app_state_machine.h",
    "App/app_safety.c",
    "App/app_safety.h",
    "App/app_telemetry.c",
    "App/app_telemetry.h",
    "App/app_params.c",
    "App/app_params.h",
    "BSP/board_map.h",
    "BSP/bsp_init.c",
    "BSP/bsp_init.h",
    "BSP/bsp_timebase.c",
    "BSP/bsp_timebase.h",
    "BSP/bsp_drive.c",
    "BSP/bsp_drive.h",
    "BSP/bsp_steering.c",
    "BSP/bsp_steering.h",
    "BSP/bsp_suction.c",
    "BSP/bsp_suction.h",
    "BSP/bsp_encoder.c",
    "BSP/bsp_encoder.h",
    "BSP/bsp_emag.c",
    "BSP/bsp_emag.h",
    "BSP/bsp_imu.c",
    "BSP/bsp_imu.h",
    "BSP/bsp_power.c",
    "BSP/bsp_power.h",
    "BSP/bsp_ui.c",
    "BSP/bsp_ui.h",
    "BSP/bsp_debug_uart.c",
    "BSP/bsp_debug_uart.h",
    "Control/ctrl_signal.c",
    "Control/ctrl_signal.h",
    "Control/ctrl_line.c",
    "Control/ctrl_line.h",
    "Control/ctrl_attitude.c",
    "Control/ctrl_attitude.h",
    "Control/ctrl_speed.c",
    "Control/ctrl_speed.h",
    "Control/ctrl_steering.c",
    "Control/ctrl_steering.h",
    "Control/ctrl_vehicle.c",
    "Control/ctrl_vehicle.h",
    "Control/ctrl_adhesion.c",
    "Control/ctrl_adhesion.h",
    "Control/ctrl_transition.c",
    "Control/ctrl_transition.h",
    "Control/ctrl_profile.c",
    "Control/ctrl_profile.h",
    "Track/track_features.c",
    "Track/track_features.h",
    "Track/track_surface_state.c",
    "Track/track_surface_state.h",
    "Track/track_state_machine.c",
    "Track/track_state_machine.h",
    "Track/track_strategy.c",
    "Track/track_strategy.h",
]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    sys.exit(1)


def assert_required_files() -> None:
    missing = [p for p in REQUIRED_FILES if not (ROOT / p).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing[:12]))


def assert_suction_default_safe() -> None:
    cfg = read("App/app_config.h")
    required = {
        "SUCTION_HW_VERIFIED": "0",
        "SUCTION_FEEDBACK_AVAILABLE": "0",
        "SUCTION_SAFE_OFF_NATIVE": "0",
        "SUCTION_IDLE_NATIVE": "0",
        "SUCTION_PRECHARGE_NATIVE": "0",
        "SUCTION_HOLD_NATIVE": "0",
        "SUCTION_BOOST_NATIVE": "0",
        "SUCTION_EMERGENCY_HOLD_NATIVE": "0",
    }
    for name, value in required.items():
        pattern = rf"#define\s+{name}\s+{value}\b"
        if not re.search(pattern, cfg):
            fail(f"{name} must default to {value} while hardware is unverified")

    suction_c = read("BSP/bsp_suction.c")
    if "SUCTION_HW_VERIFIED == 0" not in suction_c:
        fail("bsp_suction.c must explicitly force safe output when SUCTION_HW_VERIFIED == 0")
    if "bsp_suction_write_native" not in suction_c:
        fail("bsp_suction.c must centralize native actuator output")


def assert_no_fake_vacuum_feedback() -> None:
    joined = "\n".join(
        p.read_text(encoding="utf-8", errors="ignore")
        for p in ROOT.rglob("*")
        if p.is_file()
        and p.suffix.lower() in {".c", ".h", ".md"}
        and "tools/check_project.py" not in str(p)
    )
    banned = ["vacuum_pressure", "pressure_pid", "vacuum_pid", "negative_pressure_pid"]
    for token in banned:
        if token in joined:
            fail(f"banned unverified feedback name present: {token}")


def assert_safety_profiles() -> None:
    safety = read("App/app_safety.c")
    for token in ["GROUND_FAULT", "WALL_OR_UNKNOWN_FAULT", "HARD_POWER_OR_THERMAL_FAULT"]:
        if token not in safety:
            fail(f"missing safety profile token: {token}")
    if "SUCTION_EMERGENCY_HOLD" not in safety:
        fail("wall/unknown fault profile must request SUCTION_EMERGENCY_HOLD")


def assert_audit_documents_are_evidence_bound() -> None:
    audit = read("AUDIT_INPUTS.md")
    if "reference/" not in audit or "legacy_last_year/" not in audit:
        fail("AUDIT_INPUTS.md must record expected input directories")
    conflicts = read("Docs/HW_CONFLICTS.md")
    if "负压硬件未闭环确认" not in conflicts:
        fail("HW_CONFLICTS.md must record negative-pressure hardware conflict")
    build = read("Docs/BUILD_REPORT.md")
    if "未执行 Keil C251 真实编译" not in build:
        fail("BUILD_REPORT.md must not claim a real C251 build without Keil")


def main() -> int:
    assert_required_files()
    assert_suction_default_safe()
    assert_no_fake_vacuum_feedback()
    assert_safety_profiles()
    assert_audit_documents_are_evidence_bound()
    print("PASS: project integrity checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
