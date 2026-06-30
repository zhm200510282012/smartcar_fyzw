import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


class C251MemoryPlacementStaticTest(unittest.TestCase):
    def read_text(self, relative_path):
        return (REPO_ROOT / relative_path).read_text(encoding="utf-8", errors="ignore")

    def test_app_xdata_macro_is_host_safe_and_c251_xdata(self):
        header = self.read_text(Path("App") / "app_memory.h")
        self.assertIn("#ifdef HOST_SIL", header)
        self.assertRegex(header, r"#define\s+APP_XDATA\s*(?:\r?\n|$)")
        self.assertIn("#define APP_XDATA xdata", header)

    def test_required_large_persistent_objects_use_xdata(self):
        main_c = self.read_text(Path("App") / "main.c")
        emag_c = self.read_text(Path("BSP") / "bsp_emag.c")
        control_c = self.read_text(Path("App") / "app_control_tick.c")
        scheduler_c = self.read_text(Path("App") / "app_scheduler.c")
        route_event_c = self.read_text(Path("Track") / "track_route_event.c")
        features_c = self.read_text(Path("Track") / "track_features.c")
        drive_c = self.read_text(Path("BSP") / "bsp_drive.c")
        steering_c = self.read_text(Path("BSP") / "bsp_steering.c")
        suction_c = self.read_text(Path("BSP") / "bsp_suction.c")
        timers_c = self.read_text(Path("BSP") / "bsp_control_timers.c")

        self.assertIn('#include "app_memory.h"', main_c)
        self.assertRegex(main_c, r"static\s+app_context_t\s+APP_XDATA\s+g_app\s*;")
        self.assertIn('#include "../App/app_memory.h"', emag_c)
        self.assertRegex(
            emag_c,
            r"static\s+emag_frame_t\s+APP_XDATA\s+g_frame_buffers\s*\[\s*EMAG_FRAME_BUFFER_COUNT\s*\]\s*;",
        )
        for name in [
            "g_track_mode_state",
            "g_fuzzy_turn_state",
            "g_line_filter_state",
            "g_left_speed_pi",
            "g_right_speed_pi",
            "g_wall_logic",
            "g_full_course_profile",
            "g_adhesion_state",
            "g_stats",
        ]:
            self.assertRegex(control_c, rf"APP_XDATA\s+{name}\s*;")
        for name in ["g_last_sensor", "g_last_control", "g_last_track", "g_last_health", "g_last_ui"]:
            self.assertRegex(scheduler_c, rf"APP_XDATA\s+{name}\s*;")
        self.assertRegex(route_event_c, r"track_route_event_t\s+APP_XDATA\s+g_manual_event\s*;")
        for name in [
            "g_active_mask",
            "g_baseline_count",
            "g_confirm_ms",
            "g_release_ms",
            "g_cooldown_ms",
            "g_burst_hold_ms",
            "g_burst_active",
            "g_armed",
            "g_candidate",
        ]:
            self.assertRegex(features_c, rf"APP_XDATA\s+{name}\s*;")
        for name in ["g_drive_command_native", "g_drive_left_native", "g_drive_right_native"]:
            self.assertRegex(drive_c, rf"APP_XDATA\s+{name}\s*;")
        for name in ["g_steering_pulse_us", "g_steering_left_pulse_us", "g_steering_right_pulse_us"]:
            self.assertRegex(steering_c, rf"APP_XDATA\s+{name}\s*;")
        self.assertRegex(suction_c, r"suction_command_t\s+APP_XDATA\s+g_last_command\s*;")
        self.assertRegex(timers_c, r"bsp_control_timer_config_t\s+APP_XDATA\s+g_timer_config\s*;")

    def test_const_tables_do_not_use_xdata(self):
        offenders = []
        for folder in ["App", "BSP", "Control", "Track", "Drivers/Board"]:
            for path in (REPO_ROOT / folder).rglob("*.[ch]"):
                text = path.read_text(encoding="utf-8", errors="ignore")
                for line_no, line in enumerate(text.splitlines(), 1):
                    if "const" in line and "APP_XDATA" in line:
                        offenders.append(f"{path.relative_to(REPO_ROOT)}:{line_no}")
        self.assertEqual(offenders, [])

    def test_keil_stack_and_memory_model_are_not_changed(self):
        uvproj = self.read_text(Path("MDK_Project") / "smartcar_fyzw.uvproj")
        self.assertIn("<MemoryModel>3</MemoryModel>", uvproj)
        self.assertIn("<StartAddress>0x10000</StartAddress>", uvproj)
        self.assertIn("<Size>0x8000</Size>", uvproj)
        self.assertIn("<Stack></Stack>", uvproj)
        self.assertNotIn("huge", uvproj.lower())

    def test_no_huge_pointer_keyword_in_project_sources(self):
        offenders = []
        for folder in ["App", "BSP", "Control", "Track", "Drivers/Board"]:
            for path in (REPO_ROOT / folder).rglob("*.[ch]"):
                text = path.read_text(encoding="utf-8", errors="ignore")
                if re.search(r"\bhuge\b", text, re.IGNORECASE):
                    offenders.append(str(path.relative_to(REPO_ROOT)))
        self.assertEqual(offenders, [])


if __name__ == "__main__":
    unittest.main()
