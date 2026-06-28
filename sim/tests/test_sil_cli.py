import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class HostSilCliTest(unittest.TestCase):
    def test_p0_safety_gate_profiles_generate_summary(self):
        summary_path = REPO_ROOT / "sim" / "results" / "p0_safety_gate_summary.json"
        if summary_path.exists():
            summary_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_p0_safety_gate.py")],
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
        s10 = summary["scenarios"]["S10_suction_unverified"]
        self.assertEqual(s10["profile"], "guard")
        self.assertIn("SUCTION_LOCKOUT", s10["state_sequence"])
        self.assertNotIn("WALL_TRACK", s10["state_sequence"])
        self.assertEqual(s10["max_logical_suction_request"], 0)
        self.assertEqual(s10["max_hardware_suction_output"], 0)
        self.assertGreater(summary["profiles"]["logical_wall"]["max_logical_suction_request"], 0)
        self.assertEqual(summary["profiles"]["logical_wall"]["max_hardware_suction_output"], 0)


if __name__ == "__main__":
    unittest.main()
