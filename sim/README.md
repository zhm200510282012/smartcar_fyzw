# Host-SIL

Host-SIL builds the project App/Control/Track C code with a host-only BSP. It is for deterministic software validation and does not touch AI8051U registers, official driver ISR files, or real actuators.

Run the P0 safety-gate profile suite:

```powershell
python sim/scripts/run_p0_safety_gate.py
```

Outputs:

- `build/host_sil_guard_build.log`
- `build/host_sil_logical_wall_build.log`
- `sim/results/p0_safety_gate_summary.json`
- `sim/results/p0_safety_gate_summary.csv`
- `sim/results/guard_<scenario>.csv`
- `sim/results/logical_wall_<scenario>.csv`
- `Docs/SIL_RESULTS.md`

Safety boundary:

- `SUCTION_HW_VERIFIED` remains `0`.
- Guard Profile must not request logical suction or enter wall states.
- Logical Wall Profile may request theoretical host-side logical suction for state-machine coverage.
- Hardware suction output must remain `0` in Host-SIL and in the current embedded BSP until bench evidence exists.
- Host-SIL pass is not a real bench pass or real wall pass.
