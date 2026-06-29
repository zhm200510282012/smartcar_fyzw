import json
import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class FullCourseHostSilCliTest(unittest.TestCase):
    def test_full_course_scenarios_generate_summary_without_zig_fallback(self):
        build_script = REPO_ROOT / "sim" / "scripts" / "build_host_sil.py"
        script_text = build_script.read_text(encoding="utf-8")
        self.assertNotIn("zig", script_text.lower())
        self.assertNotIn("python-zig", script_text.lower())

        summary_path = REPO_ROOT / "sim" / "results" / "full_course_summary.json"
        if summary_path.exists():
            summary_path.unlink()

        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / "sim" / "scripts" / "run_full_course_sil.py")],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue(summary_path.exists(), result.stdout)
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        self.assertEqual(summary["scenario_count"], 20)
        self.assertEqual(summary["failed"], 0)
        self.assertEqual(summary["safety"]["hardware_suction_output_max"], 0)
        self.assertEqual(summary["safety"]["real_wall_pass"], False)
        self.assertIn("S20_control_period_overrun_fault", summary["scenarios"])


if __name__ == "__main__":
    unittest.main()
