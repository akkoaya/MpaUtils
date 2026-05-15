#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DOWNLOAD_HELPER = ROOT / "tools" / "mpadeps-download.py"


def resolve_tarball_dir() -> Path:
    candidates = (
        ROOT.parent / "MpaDeps" / "tarball",
        ROOT.parent.parent / "MpaDeps" / "tarball",
    )
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def main(argv: list[str]) -> int:
    if len(argv) > 2:
        raise SystemExit("usage: mpadeps-extract.py [triplet]")

    if not DOWNLOAD_HELPER.exists():
        raise FileNotFoundError(f"Expected helper at {DOWNLOAD_HELPER}")

    tarball_dir = resolve_tarball_dir()
    cmd = [
        sys.executable,
        str(DOWNLOAD_HELPER),
        "--from-dir",
        str(tarball_dir),
        "--dest",
        str(ROOT / "MpaDeps"),
    ]
    if len(argv) == 2:
        cmd.append(argv[1])
    return subprocess.call(cmd, cwd=str(ROOT))


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
