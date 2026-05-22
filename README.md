# MpaUtils

`MpaUtils` is the shared utility library and dependency bridge used by
`MpaFrameWork`.

It mirrors the role of `MaaUtils` in `MaaFramework`, but is wired for the
project-owned `MpaDeps` release-download flow.

## Responsibilities

- shared native utility code: logging, encoding, platform/runtime helpers,
  files, buffers, image helpers, IO streams, UUID, JSON extensions, dynamic
  library loading
- default third-party dependency bridge for `OpenCV`, `Boost`,
  `ONNXRuntime`, and `fastdeploy_ppocr`
- CMake entrypoint consumed by `MpaFrameWork` through
  `source/MpaUtils/MpaUtils.cmake`

## Dependency Model

`MpaUtils` is expected to be consumed from `https://github.com/akkoaya/MpaUtils`
as the `source/MpaUtils` submodule in `MpaFrameWork`.

Expected layout when consumed from `MpaFrameWork`:

```text
MpaFrameWork/
`-- source/
    `-- MpaUtils/
        |-- MpaDeps/
        |   `-- mpadeps.cmake
        `-- MpaUtils.cmake
```

Bootstrap `MpaDeps` into the `MpaUtils` checkout with:

```powershell
python tools/mpadeps-download.py --repo akkoaya/MpaDeps
```

The helper resolves the latest published `MpaDeps` release by default. Pass
`--version <tag>` when a reproducible dependency snapshot is required.

For local release-asset validation:

```powershell
python tools/mpadeps-download.py --from-dir ..\MpaDeps\tarball
```

The example above assumes `MpaUtils` and `MpaDeps` are sibling checkouts.

## Local Helper

`tools/mpadeps-download.py` in this repository is a standalone bootstrap
helper. By default it extracts release assets into `MpaUtils/MpaDeps`.
