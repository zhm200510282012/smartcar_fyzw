import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class FuzzyPidHostSilCliTest(unittest.TestCase):
    def test_fuzzy_pid_scenarios_generate_summary_from_c_runner(self):
        build_script = REPO_ROOT / "sim" / "scripts" / "build_host_sil.py"
        script_text = build_script.read_text(encoding="utf-8")
        self.assertNotIn("zig", script_text.lower())
        self.assertNotIn("python-zig", script_text.lower())

        summary_path = REPO_ROOT / "sim" / "results" / "fuzzy_pid_summary.json"
        csv_path = REPO_ROOT / "sim" / "results" / "fuzzy_pid_summary.csv"
        if summary_path.exists():
            summary_path.unlink()
        if csv_path.exists():
            csv_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_fuzzy_pid_sil.py")],
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
        for scenario in [f"S{index:02d}" for index in range(29, 39)]:
            self.assertIn(scenario, summary["scenarios"])
            self.assertTrue(summary["scenarios"][scenario]["pass"], scenario)


if __name__ == "__main__":
    unittest.main()
