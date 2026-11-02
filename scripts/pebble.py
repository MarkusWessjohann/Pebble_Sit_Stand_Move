import subprocess
import sys
from pathlib import Path


def main() -> int:
    project_root = Path(__file__).resolve().parent.parent
    sdk_version = (project_root / ".pebble-sdk-version").read_text().strip()
    sdk_list = subprocess.run(
        ["uv", "run", "--locked", "pebble", "sdk", "list"],
        cwd=project_root,
        capture_output=True,
        text=True,
    )
    if sdk_list.returncode:
        sys.stderr.write(sdk_list.stderr)
        return sdk_list.returncode

    expected = f"{sdk_version} (active)"
    if not any(
        line.strip() in (expected, f"Installed SDKs: {expected}")
        for line in sdk_list.stdout.splitlines()
    ):
        print(
            f"Pebble SDK {sdk_version} must be active before this project command.\n"
            f"Run `pebble sdk list` and activate {sdk_version} in the host Pebble SDK manager.",
            file=sys.stderr,
        )
        return 1

    if len(sys.argv) < 2:
        print("Usage: python scripts/pebble.py <pebble command> [args...]", file=sys.stderr)
        return 2

    return subprocess.run(
        ["uv", "run", "--locked", "pebble", *sys.argv[1:]],
        cwd=project_root,
    ).returncode


if __name__ == "__main__":
    sys.exit(main())
