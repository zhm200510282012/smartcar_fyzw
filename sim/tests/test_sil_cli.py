import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class HostSilCliTest(unittest.TestCase):
    def test_run_sil_generates_machine_readable_summary(self):
        summary_path = REPO_ROOT / "sim" / "results" / "summary.json"
        if summary_path.exists():
            summary_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_sil.py")],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(summary_path.exists(), result.stdout)
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        self.assertEqual(summary["scenario_count"], 13)
        self.assertEqual(summary["failed"], 0)
        self.assertIn("S10_suction_unverified", summary["scenarios"])


if __name__ == "__main__":
    unittest.main()
