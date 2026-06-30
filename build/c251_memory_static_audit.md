# C251 Memory Static Audit

Checks:

| Check | Result |
|---|---|
| `APP_XDATA` is empty under `HOST_SIL` | PASS |
| `APP_XDATA` expands to `xdata` for real C251 | PASS |
| `App/main.c::g_app` is marked `APP_XDATA` | PASS |
| `BSP/bsp_emag.c::g_frame_buffers` is marked `APP_XDATA` | PASS |
| `BSP/bsp_emag.c` keeps front/write/reader indexes as volatile internal-data objects | PASS |
| `?STACK` configuration was not reduced | PASS |
| Keil `MemoryModel` remains `3` | PASS |
| XRAM range remains `0x10000` size `0x8000` | PASS |
| `huge` pointer keyword is not used in project sources | PASS |
| No `const APP_XDATA` objects remain in project sources | PASS |
| Additional non-const persistent state was migrated after Keil reported `edata+hdata=1627` | PASS |
| `BSP/bsp_imu.c` diff hash stayed unchanged from turn start | PASS |
| No `git add .` or `git add -A` was used | PASS |

Verification commands for this audit:

```text
python -m unittest sim.tests.test_c251_memory_placement_static
git diff --check
python -m unittest discover -s sim/tests
git diff -- BSP/bsp_imu.c
git status --short
```

Keil C251 note:

```text
Codex did not run Keil C251.
The user must confirm final Program Size and link result on the local Keil installation.
```
