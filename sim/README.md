# Host-SIL

Host-SIL builds the project App/Control/Track C code with a host-only BSP. It is for deterministic software validation and does not touch AI8051U registers, official driver ISR files, or real actuators.

Run all scenarios:

```powershell
python sim/scripts/run_sil.py
```

Outputs:

- `build/host_sil_build.log`
- `sim/results/summary.json`
- `sim/results/summary.csv`
- `sim/results/<scenario>.csv`
- `Docs/SIL_RESULTS.md`

Safety boundary:

- `SUCTION_HW_VERIFIED` remains `0`.
- Host-SIL may request logical suction modes in software states.
- Hardware suction output must remain `0` in Host-SIL and in the current embedded BSP until bench evidence exists.
- Host-SIL pass is not a real bench pass or real wall pass.
