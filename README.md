# MpaUtils

`MpaUtils` is the shared utility library and dependency bridge used by
`MpaFrameWork`.

It mirrors the role of `MaaUtils` in `MaaFramework`, but is wired for the
project-owned `MpaDeps` release-download flow instead of an in-tree `MaaDeps`
submodule.

## Responsibilities

- shared native utility code: logging, encoding, platform/runtime helpers,
  files, buffers, image helpers, IO streams, UUID, JSON extensions, dynamic
  library loading
- default third-party dependency bridge for `OpenCV`, `Boost`,
  `ONNXRuntime`, and `fastdeploy_ppocr`
- CMake entrypoint consumed by `MpaFrameWork` through
  `source/MpaUtils/MpaUtils.cmake`

## Dependency Model

`MpaUtils` does not vendor `MpaDeps` as a submodule.

Expected layout when consumed from `MpaFrameWork`:

```text
MpaFrameWork/
├── MpaDeps/
│   └── mpadeps.cmake
└── source/
    └── MpaUtils/
        └── MpaUtils.cmake
```

Bootstrap `MpaDeps` from the main repository root with:

```powershell
python tools/mpadeps-download.py --repo <owner/repo> --version <tag>
```

For local release-asset validation:

```powershell
python tools/mpadeps-download.py --from-dir ..\MpaDeps\tarball
```

## Local Helper

`tools/mpadeps-download.py` in this repository is a thin wrapper that forwards
to the shared bootstrap helper in the adjacent `MpaFrameWork` checkout.
