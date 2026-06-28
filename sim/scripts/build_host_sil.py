import glob
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = REPO_ROOT / "build"
EXE_PATH = REPO_ROOT / "sim" / "host" / ("host_sil.exe" if os.name == "nt" else "host_sil")
BUILD_LOG = BUILD_DIR / "host_sil_build.log"


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


def build():
    compiler = find_compiler()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    if compiler is None:
        BUILD_LOG.write_text("No host C compiler found. Tried HOST_SIL_CC, gcc, clang, cc, zig cc, python-zig.exe cc.\n", encoding="utf-8")
        return 2

    cmd = (
        compiler
        + [
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-DHOST_SIL=1",
            "-I.",
            "-IApp",
            "-IBSP",
            "-IControl",
            "-ITrack",
            "-Isim/host",
            "-o",
            str(EXE_PATH),
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

    BUILD_LOG.write_text(
        "COMMAND:\n"
        + " ".join(cmd)
        + "\n\nOUTPUT:\n"
        + result.stdout
        + f"\nEXIT_CODE: {result.returncode}\n",
        encoding="utf-8",
    )

    if result.returncode == 0:
        print(EXE_PATH)
    else:
        print(BUILD_LOG)
    return result.returncode


def main():
    return build()


if __name__ == "__main__":
    sys.exit(main())
