import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class RealControlChainHostSilCliTest(unittest.TestCase):
    def test_real_control_chain_scenarios_generate_summary_from_c_runner(self):
        summary_path = REPO_ROOT / "sim" / "results" / "real_control_chain_summary.json"
        csv_path = REPO_ROOT / "sim" / "results" / "real_control_chain_summary.csv"
        if summary_path.exists():
            summary_path.unlink()
        if csv_path.exists():
            csv_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_real_control_chain_sil.py")],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(summary_path.exists(), result.stdout)
        self.assertTrue(csv_path.exists(), result.stdout)
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        self.assertEqual(summary["scenario_count"], 10)
        self.assertEqual(summary["passed"], 10)
        self.assertEqual(summary["failed"], 0)
        self.assertEqual(summary["safety"]["hardware_suction_output_max"], 0)
        for scenario in [f"S{index:02d}" for index in range(39, 49)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)


if __name__ == "__main__":
    unittest.main()
