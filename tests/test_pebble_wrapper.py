import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "pebble.py"
SPEC = importlib.util.spec_from_file_location("pebble_wrapper", SCRIPT)
pebble_wrapper = importlib.util.module_from_spec(SPEC)
sys.modules["pebble_wrapper"] = pebble_wrapper
SPEC.loader.exec_module(pebble_wrapper)


class PebbleWrapperTest(unittest.TestCase):
    @patch("pebble_wrapper.subprocess.run")
    @patch("pebble_wrapper.sys.argv", ["pebble.py", "build"])
    def test_runs_project_command_when_pinned_sdk_is_active(self, run):
        run.side_effect = [
            type("Result", (), {"returncode": 0, "stdout": "Installed SDKs:\n4.33.1 (active)\n", "stderr": ""})(),
            type("Result", (), {"returncode": 0})(),
        ]

        self.assertEqual(pebble_wrapper.main(), 0)
        self.assertEqual(run.call_args_list[0].args[0], ["uv", "run", "--locked", "pebble", "sdk", "list"])
        self.assertEqual(run.call_args_list[1].args[0], ["uv", "run", "--locked", "pebble", "build"])

    @patch("pebble_wrapper.subprocess.run")
    @patch("pebble_wrapper.sys.argv", ["pebble.py", "build"])
    def test_runs_project_command_when_active_sdk_is_on_installed_sdks_line(self, run):
        run.side_effect = [
            type("Result", (), {"returncode": 0, "stdout": "Installed SDKs: 4.33.1 (active)\n", "stderr": ""})(),
            type("Result", (), {"returncode": 0})(),
        ]

        self.assertEqual(pebble_wrapper.main(), 0)
        self.assertEqual(run.call_count, 2)

    @patch("pebble_wrapper.subprocess.run")
    @patch("pebble_wrapper.sys.argv", ["pebble.py", "install", "--emulator", "emery"])
    def test_blocks_command_when_sdk_pin_is_not_active(self, run):
        run.return_value = type(
            "Result", (), {"returncode": 0, "stdout": "Installed SDKs:\n4.33.1\n", "stderr": ""}
        )()

        self.assertEqual(pebble_wrapper.main(), 1)
        run.assert_called_once()

    @patch("pebble_wrapper.subprocess.run")
    @patch("pebble_wrapper.sys.argv", ["pebble.py", "build"])
    def test_propagates_sdk_list_failure(self, run):
        run.return_value = type("Result", (), {"returncode": 17, "stdout": "", "stderr": "unavailable"})()

        self.assertEqual(pebble_wrapper.main(), 17)
        run.assert_called_once()


if __name__ == "__main__":
    unittest.main()
