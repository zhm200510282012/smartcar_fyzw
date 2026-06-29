import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class RealRuntimeGapClosureHostSilCliTest(unittest.TestCase):
    def test_real_runtime_gap_scenarios_generate_summary_from_c_runner(self):
        summary_path = REPO_ROOT / "sim" / "results" / "real_runtime_gap_closure_summary.json"
        csv_path = REPO_ROOT / "sim" / "results" / "real_runtime_gap_closure_summary.csv"
        if summary_path.exists():
            summary_path.unlink()
        if csv_path.exists():
            csv_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_real_runtime_gap_closure_sil.py")],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(summary_path.exists(), result.stdout)
        self.assertTrue(csv_path.exists(), result.stdout)
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        self.assertEqual(summary["scenario_count"], 15)
        self.assertEqual(summary["passed"], 15)
        self.assertEqual(summary["failed"], 0)
        self.assertEqual(summary["safety"]["fan_esc_physical_output_enable"], 0)
        self.assertEqual(summary["safety"]["wall_run_enable"], 0)
        self.assertEqual(summary["safety"]["suction_hw_verified"], 0)
        self.assertEqual(summary["safety"]["route_progress_script_enable"], 0)
        self.assertFalse(summary["safety"]["real_wall_pass"])
        for scenario in [f"RUNTIME{index:02d}" for index in range(1, 16)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)


if __name__ == "__main__":
    unittest.main()
