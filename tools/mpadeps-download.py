#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MAIN_REPO_DOWNLOAD = ROOT.parent / "MpaFrameWork" / "tools" / "mpadeps-download.py"


def main(argv: list[str]) -> int:
    if not MAIN_REPO_DOWNLOAD.exists():
        raise FileNotFoundError(
            f"Expected shared bootstrap helper at {MAIN_REPO_DOWNLOAD}. "
            "Keep MpaUtils and MpaFrameWork adjacent, or invoke MpaFrameWork/tools/mpadeps-download.py directly."
        )

    env = os.environ.copy()
    env.setdefault("PYTHONUTF8", "1")
    cmd = [sys.executable, str(MAIN_REPO_DOWNLOAD), *argv[1:]]
    return subprocess.call(cmd, cwd=str(ROOT), env=env)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
