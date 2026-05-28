#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DEST = ROOT / "MpaDeps"
DEFAULT_REPO = "akkoaya/MpaDeps"
DEFAULT_VERSION = "v0.1.1"


def detect_host_triplet() -> str:
    import platform

    machine = platform.machine().lower()
    system = platform.system().lower()

    if machine in {"amd64", "x86_64"}:
        machine = "x64"
    elif machine in {"x86", "i386", "i486", "i586", "i686"}:
        machine = "x86"
    elif machine in {"armv7l", "armv7a", "arm", "arm32"}:
        machine = "arm"
    elif machine in {"arm64", "armv8l", "aarch64"}:
        machine = "arm64"
    else:
        raise Exception("unsupported architecture: " + machine)

    if system in {"windows", "linux"}:
        pass
    elif "mingw" in system or "cygwin" in system:
        system = "windows"
    elif system == "darwin":
        system = "osx"
    else:
        raise Exception("unsupported system: " + system)

    return f"mpa-{machine}-{system}"


def format_size(num: float, suffix: str = "B") -> str:
    for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
        if abs(num) < 1024.0:
            return f"{num:3.1f}{unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f}Yi{suffix}"


class ProgressHook:
    def __init__(self) -> None:
        self.downloaded = 0
        self.last_print = 0.0

    def __call__(self, block: int, chunk: int, total: int) -> None:
        self.downloaded += chunk
        now = time.monotonic()
        if now - self.last_print >= 0.5 or (total > 0 and self.downloaded == total):
            self.last_print = now
            if total > 0:
                print(
                    f"\r [{self.downloaded / total * 100.0:3.1f}%] "
                    f"{format_size(self.downloaded)} / {format_size(total)}      \r",
                    end="",
                )
        if total > 0 and self.downloaded == total:
            print("")


def sanitize_filename(filename: str) -> str:
    import platform

    system = platform.system()
    if system == "Windows":
        return filename.translate(str.maketrans('/\\:"?*|\0', "________")).rstrip(".")
    if system == "Darwin":
        return filename.translate(str.maketrans("/:\0", "___"))
    return filename.translate(str.maketrans("/\0", "__"))


def retry_urlopen(req: urllib.request.Request) -> bytes:
    for _ in range(5):
        try:
            with urllib.request.urlopen(req) as resp:
                return resp.read()
        except urllib.error.HTTPError as exc:
            if exc.status == 403 and exc.headers.get("x-ratelimit-remaining") == "0":
                now = time.time()
                reset_time = now + 10
                try:
                    reset_time = int(exc.headers.get("x-ratelimit-reset", "0"))
                except ValueError:
                    pass
                reset_time = max(reset_time, now + 10)
                wait_seconds = reset_time - now
                print(f"rate limit exceeded, retrying after {wait_seconds:.1f} seconds")
                time.sleep(wait_seconds)
                continue
            raise
    raise RuntimeError("failed to query release after retries")


def local_digest_matches(path: Path, digest: str) -> bool:
    if not path.exists() or not digest.startswith("sha256:"):
        return False
    hasher = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(8192)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest() == digest[len("sha256:") :]


def clear_destination(dest_dir: Path) -> None:
    if dest_dir.exists():
        shutil.rmtree(dest_dir)
    dest_dir.mkdir(parents=True, exist_ok=True)


def asset_names(triplet: str) -> tuple[str, str]:
    return (
        f"MpaDeps-{triplet}-devel.tar.xz",
        f"MpaDeps-{triplet}-runtime.tar.xz",
    )


def find_local_archives(archive_dir: Path, triplet: str) -> list[Path]:
    devel_name, runtime_name = asset_names(triplet)
    devel = archive_dir / devel_name
    runtime = archive_dir / runtime_name
    missing = [str(path) for path in (devel, runtime) if not path.exists()]
    if missing:
        raise FileNotFoundError(
            "local MpaDeps archives were not found:\n  " + "\n  ".join(missing)
        )
    return [devel, runtime]


def extract_archives(dest_dir: Path, archives: list[Path]) -> None:
    clear_destination(dest_dir)
    (dest_dir / "tarball").mkdir(parents=True, exist_ok=True)
    for archive in archives:
        cached_path = dest_dir / "tarball" / archive.name
        if archive.resolve() != cached_path.resolve():
            shutil.copy2(archive, cached_path)
        print("extracting", archive.name)
        shutil.unpack_archive(str(archive), str(dest_dir))


def download_release_assets(
    repo: str,
    version: str,
    triplet: str,
    dest_dir: Path,
    token: str | None,
) -> None:
    req = urllib.request.Request(
        f"https://api.github.com/repos/{repo}/releases/tags/{version}"
    )
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    release = json.loads(retry_urlopen(req))

    expected = set(asset_names(triplet))
    assets = {}
    for asset in release["assets"]:
        name = asset["name"]
        if name in expected:
            assets[name] = asset

    missing = [name for name in expected if name not in assets]
    if missing:
        raise RuntimeError(
            f"release {repo}@{version} does not contain required assets for {triplet}: "
            + ", ".join(missing)
        )

    clear_destination(dest_dir)
    cache_dir = dest_dir / "tarball"
    cache_dir.mkdir(parents=True, exist_ok=True)

    for name in asset_names(triplet):
        asset = assets[name]
        url = asset["browser_download_url"]
        local_path = cache_dir / sanitize_filename(name)
        if local_digest_matches(local_path, asset.get("digest", "")):
            print("reusing matched digest", name)
        else:
            print("downloading", url)
            urllib.request.urlretrieve(url, local_path, reporthook=ProgressHook())
        print("extracting", name)
        shutil.unpack_archive(str(local_path), str(dest_dir))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Bootstrap repo-local MpaDeps by extracting release assets from a "
            "published standalone MpaDeps repository or from local tarball assets."
        )
    )
    parser.add_argument("triplet", nargs="?", default=detect_host_triplet())
    parser.add_argument(
        "--repo",
        default=os.environ.get("MPADEPS_REPO", DEFAULT_REPO),
        help="Published standalone MpaDeps repository in owner/name form.",
    )
    parser.add_argument(
        "--version",
        default=os.environ.get("MPADEPS_VERSION", DEFAULT_VERSION),
        help="Published standalone MpaDeps release tag.",
    )
    parser.add_argument("--dest", default=str(DEFAULT_DEST))
    parser.add_argument(
        "--from-dir",
        default="",
        help="Extract archives from a local directory instead of GitHub release downloads.",
    )
    return parser


def main(argv: list[str]) -> int:
    parser = build_parser()
    args = parser.parse_args(argv[1:])

    triplet = args.triplet
    dest_dir = Path(args.dest).resolve()
    local_dir = Path(args.from_dir).resolve() if args.from_dir else None

    if local_dir:
        extract_archives(dest_dir, find_local_archives(local_dir, triplet))
        print(f"MpaDeps extracted to {dest_dir}")
        return 0

    if not args.repo or not args.version:
        parser.error(
            "--repo and --version are required unless --from-dir is given. "
            "Set MPADEPS_REPO/MPADEPS_VERSION or pass published release metadata explicitly."
        )

    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
    download_release_assets(args.repo, args.version, triplet, dest_dir, token)
    print(f"MpaDeps extracted to {dest_dir}")
    return 0


def download_main(triplet: str, repo: str, version: str) -> int:
    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
    download_release_assets(repo, version, triplet, DEFAULT_DEST, token)
    print(f"MpaDeps extracted to {DEFAULT_DEST}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
