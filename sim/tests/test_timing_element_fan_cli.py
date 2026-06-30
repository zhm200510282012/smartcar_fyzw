import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class TimingElementFanHostSilCliTest(unittest.TestCase):
    def test_timing_element_fan_scenarios_generate_summary_from_c_runner(self):
        summary_path = REPO_ROOT / "sim" / "results" / "timing_element_fan_summary.json"
        csv_path = REPO_ROOT / "sim" / "results" / "timing_element_fan_summary.csv"
        if summary_path.exists():
            summary_path.unlink()
        if csv_path.exists():
            csv_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_timing_element_fan_sil.py")],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(summary_path.exists(), result.stdout)
        self.assertTrue(csv_path.exists(), result.stdout)
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        self.assertEqual(summary["scenario_count"], 18)
        self.assertEqual(summary["passed"], 18)
        self.assertEqual(summary["failed"], 0)
        self.assertEqual(summary["safety"]["fan_esc_physical_output_enable"], 0)
        self.assertEqual(summary["safety"]["wall_run_enable"], 0)
        self.assertEqual(summary["safety"]["suction_hw_verified"], 0)
        self.assertEqual(summary["safety"]["fan_bench_test_enable"], 0)
        self.assertFalse(summary["safety"]["real_wall_pass"])
        self.assertFalse(summary["safety"]["real_bench_pass"])
        for scenario in [f"TIMER{index:02d}" for index in range(1, 7)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)
        for scenario in [f"ELEMENT{index:02d}" for index in range(1, 9)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)
        for scenario in [f"FAN{index:02d}" for index in range(1, 5)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)


if __name__ == "__main__":
    unittest.main()
