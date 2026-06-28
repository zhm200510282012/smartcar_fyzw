import glob
import argparse
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / "build"


SOURCES = [
    "sim/host/sil_runner.c",
    "sim/host/host_bsp.c",
    "App/app_scheduler.c",
    "App/app_state_machine.c",
    "App/app_safety.c",
    "App/app_telemetry.c",
    "App/app_params.c",
    "Control/ctrl_adhesion.c",
    "Control/ctrl_attitude.c",
    "Control/ctrl_line.c",
    "Control/ctrl_profile.c",
    "Control/ctrl_signal.c",
    "Control/ctrl_speed.c",
    "Control/ctrl_steering.c",
    "Control/ctrl_transition.c",
    "Control/ctrl_vehicle.c",
    "Track/track_features.c",
    "Track/track_state_machine.c",
    "Track/track_strategy.c",
    "Track/track_surface_state.c",
]


def _existing_command(candidate):
    if not candidate:
        return None
    parts = candidate.split()
    exe = shutil.which(parts[0]) or (parts[0] if Path(parts[0]).exists() else None)
    if exe:
        return [exe] + parts[1:]
    return None


def _python_zig_candidates():
    candidates = []
    scripts_dir = sysconfig.get_path("scripts")
    if scripts_dir:
        candidates.append(Path(scripts_dir) / "python-zig.exe")
        candidates.append(Path(scripts_dir) / "python-zig")
    appdata = os.environ.get("APPDATA")
    if appdata:
        candidates.extend(Path(p) for p in glob.glob(str(Path(appdata) / "Python" / "Python*" / "Scripts" / "python-zig.exe")))
    return candidates


def find_compiler():
    env_cc = _existing_command(os.environ.get("HOST_SIL_CC", ""))
    if env_cc:
        return env_cc

    for name in ("gcc", "clang", "cc"):
        path = shutil.which(name)
        if path:
            return [path]

    zig = shutil.which("zig")
    if zig:
        return [zig, "cc"]

    for candidate in _python_zig_candidates():
        if candidate.exists():
            return [str(candidate), "cc"]

    return None


def profile_flags(profile):
    if profile == "guard":
        return ["-DHOST_SIL_GUARD_PROFILE=1"]
    if profile == "logical_wall":
        return ["-DHOST_SIL_LOGICAL_WALL_PROFILE=1"]
    raise ValueError(f"unknown profile {profile}")


def profile_exe_path(profile):
    suffix = ".exe" if os.name == "nt" else ""
    return REPO_ROOT / "sim" / "host" / f"host_sil_{profile}{suffix}"


def profile_log_path(profile):
    return BUILD_DIR / f"host_sil_{profile}_build.log"


def build(profile="guard"):
    compiler = find_compiler()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    build_log = profile_log_path(profile)
    exe_path = profile_exe_path(profile)
    if compiler is None:
        build_log.write_text("No host C compiler found. Tried HOST_SIL_CC, gcc, clang, cc, zig cc, python-zig.exe cc.\n", encoding="utf-8")
        return 2

    cmd = (
        compiler
        + [
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-DHOST_SIL=1",
            *profile_flags(profile),
            "-I.",
            "-IApp",
            "-IBSP",
            "-IControl",
            "-ITrack",
            "-Isim/host",
            "-o",
            str(exe_path),
        ]
        + [str(REPO_ROOT / source) for source in SOURCES]
    )

    result = subprocess.run(
        cmd,
        cwd=str(REPO_ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    build_log.write_text(
        "COMMAND:\n"
        + " ".join(cmd)
        + "\n\nOUTPUT:\n"
        + result.stdout
        + f"\nEXIT_CODE: {result.returncode}\n",
        encoding="utf-8",
    )

    if result.returncode == 0:
        print(exe_path)
    else:
        print(build_log)
    return result.returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=("guard", "logical_wall"), default="guard")
    args = parser.parse_args()
    return build(args.profile)


if __name__ == "__main__":
    sys.exit(main())
