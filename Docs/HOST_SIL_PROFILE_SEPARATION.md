# Host-SIL Profile Separation

## Guard Profile

Build command:

```powershell
python sim/scripts/build_host_sil.py --profile guard
```

Run command:

```powershell
python sim/scripts/run_p0_safety_gate.py
```

Compile-time properties:

- `HOST_SIL_GUARD_PROFILE=1`
- `HOST_SIL_LOGICAL_SUCTION_AVAILABLE=0`
- `SUCTION_HW_VERIFIED=0`
- `APP_WALL_STATE_CAPABLE=0`

Purpose:

- Verify that unverified suction hardware cannot enter wall-related states.
- Verify `S10_suction_unverified` reaches `APP_STATE_SUCTION_LOCKOUT`.
- Verify logical suction request and hardware suction output both remain zero.

## Logical Wall Profile

Build command:

```powershell
python sim/scripts/build_host_sil.py --profile logical_wall
```

Run command:

```powershell
python sim/scripts/run_p0_safety_gate.py
```

Compile-time properties:

- `HOST_SIL_LOGICAL_WALL_PROFILE=1`
- `HOST_SIL_LOGICAL_SUCTION_AVAILABLE=1`
- `SUCTION_HW_VERIFIED=0`
- `APP_WALL_STATE_CAPABLE=1`

Purpose:

- Verify only Host-SIL state-machine flow, logical suction requests, and fault handling.
- Allow nonzero logical suction request values in CSV output.
- Keep hardware suction output at zero.

This profile does not validate ESC wiring, fan response, suction polarity, adhesion, or real wall capability. It does not change the C251 target profile or `.uvproj` defines.
