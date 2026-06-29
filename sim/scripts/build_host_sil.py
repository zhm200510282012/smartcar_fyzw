import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / "build"


SOURCES = [
    "sim/host/sil_runner.c",
    "sim/host/host_bsp.c",
    "App/app_scheduler.c",
    "App/app_output_arbitration.c",
    "App/app_state_machine.c",
    "App/app_safety.c",
    "App/app_telemetry.c",
    "App/app_params.c",
    "Control/ctrl_adhesion.c",
    "Control/ctrl_attitude.c",
    "Control/ctrl_line.c",
    "Control/ctrl_profile.c",
    "Control/ctrl_signal.c",
    "Control/ctrl_fuzzy_pid.c",
    "Control/ctrl_fuzzy_steering.c",
    "Control/ctrl_speed.c",
    "Control/ctrl_steering.c",
    "Control/ctrl_transition.c",
    "Control/ctrl_vehicle.c",
    "Track/track_features.c",
    "Track/track_route_profile.c",
    "Track/track_state_machine.c",
    "Track/track_strategy.c",
    "Track/track_surface_state.c",
]

KNOWN_COMPILER_PATHS = [
    r"D:\Qt\Tools\mingw810_64\bin\gcc.exe",
    r"D:\Qt\Tools\mingw810_32\bin\gcc.exe",
    r"D:\VIVADO19\Vivado\2019.1\msys64\mingw64\bin\gcc.exe",
    r"D:\application file\Dev-Cpp\MinGW64\bin\gcc.exe",
    r"D:\VIVADO19\Vivado\2019.1\msys64\mingw64\bin\clang.exe",
    r"D:\Matlab\matlab\toolbox\sldrt\clang\win64\clang.exe",
]


def _existing_command(candidate):
    if not candidate:
        return None
    parts = shlex.split(candidate, posix=False)
    exe = shutil.which(parts[0]) or (parts[0] if Path(parts[0]).exists() else None)
    if exe:
        return [exe] + parts[1:]
    return None


def find_compiler():
    env_cc = _existing_command(os.environ.get("HOST_SIL_CC", ""))
    if env_cc:
        return env_cc

    for name in ("cl", "gcc", "clang", "cc"):
        path = shutil.which(name)
        if path:
            return [path]

    for candidate in KNOWN_COMPILER_PATHS:
        path = Path(candidate)
        if path.exists():
            return [str(path)]

    return None


def compiler_kind(compiler):
    exe_name = Path(compiler[0]).name.lower()
    if exe_name == "cl.exe" or exe_name == "cl":
        return "msvc"
    return "gcc"


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
        build_log.write_text(
            "No host C compiler found. Tried HOST_SIL_CC, cl, gcc, clang, cc, and known existing local compiler paths.\n",
            encoding="utf-8",
        )
        return 2

    source_paths = [str(REPO_ROOT / source) for source in SOURCES]
    if compiler_kind(compiler) == "msvc":
        cmd = (
            compiler
            + [
                "/nologo",
                "/W4",
                "/WX",
                "/DHOST_SIL=1",
                *(flag.replace("-D", "/D") for flag in profile_flags(profile)),
                "/I.",
                "/IApp",
                "/IBSP",
                "/IControl",
                "/ITrack",
                "/Isim/host",
            ]
            + source_paths
            + ["/Fe:" + str(exe_path)]
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
            + source_paths
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
