import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
PROJECT_SOURCE_DIRS = ["App", "BSP", "Control", "Track", "MDK_Project"]


def read_project_texts():
    suffixes = {".c", ".h", ".uvproj"}
    for folder in PROJECT_SOURCE_DIRS:
        root = REPO_ROOT / folder
        for path in root.rglob("*"):
            if path.suffix in suffixes:
                yield path, path.read_text(encoding="utf-8", errors="ignore")


class C251CompatibilityStaticTest(unittest.TestCase):
    def test_no_project_timer11_or_legacy_ai8051u_header(self):
        offenders = []
        for path, text in read_project_texts():
            if 'include "AI8051U.h"' in text:
                offenders.append(f"{path.relative_to(REPO_ROOT)}: AI8051U.h")
            if "TMR11_VECTOR" in text:
                offenders.append(f"{path.relative_to(REPO_ROOT)}: TMR11_VECTOR")
            if "NVIC_Timer11_Init" in text:
                offenders.append(f"{path.relative_to(REPO_ROOT)}: NVIC_Timer11_Init")
        self.assertEqual(offenders, [])

    def test_no_plain_bit_variable_in_project_code(self):
        offenders = []
        pattern = re.compile(r"\b(?:u8|u16|u32|s8|s16|s32|int|char)\s+bit\b")
        for path, text in read_project_texts():
            if path.suffix not in {".c", ".h"}:
                continue
            for line_no, line in enumerate(text.splitlines(), 1):
                if pattern.search(line):
                    offenders.append(f"{path.relative_to(REPO_ROOT)}:{line_no}")
        self.assertEqual(offenders, [])

    def test_bsp_emag_uses_output_parameter_sample_api(self):
        header = (REPO_ROOT / "BSP" / "bsp_emag.h").read_text(encoding="utf-8")
        source = (REPO_ROOT / "BSP" / "bsp_emag.c").read_text(encoding="utf-8")
        self.assertIn("void bsp_emag_sample_from_frame(", header)
        self.assertIn("void bsp_emag_read(", header)
        self.assertIn("void bsp_emag_last_sample(", header)
        self.assertNotIn("emag_sample_t bsp_emag_sample_from_frame(", source)
        self.assertNotIn("emag_sample_t bsp_emag_read(", source)
        self.assertNotIn("emag_sample_t bsp_emag_last_sample(", source)

    def test_app_control_tick_real_branch_declaration_before_early_return(self):
        source = (REPO_ROOT / "App" / "app_control_tick.c").read_text(encoding="utf-8")
        fn_match = re.search(
            r"void\s+app_control_tick_sensor_isr\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(fn_match)
        body = fn_match.group("body")
        self.assertLess(body.find("emag_frame_t frame;"), body.find("if (ctx == 0)"))


if __name__ == "__main__":
    unittest.main()
