#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


SUB_PATH = Path(__file__).parent
sys.path.append(str(SUB_PATH))

from mpadeps_download import main


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
