# Photo Mode — IGCSDOF V20 integration

This folder contains only the files that must be added or changed in a fork of
`powerof3/PhotoMode`.

## Files to add

- `src/IGCSBridge/Bridge.h`
- `src/IGCSBridge/Bridge.cpp`

## Existing files to replace / merge

- `src/Hooks.cpp`
- `src/PhotoMode/Manager.cpp`
- `cmake/sourcelist.cmake`
- `cmake/headerlist.cmake`

The relevant integration lines are annotated with `IGCSDOF:` comments.

## Recommended Git workflow

```bat
git checkout -b feature/igcsdof-direct-bridge
```

Copy the six files over the fork, then review:

```bat
git status
git diff -- src/IGCSBridge src/Hooks.cpp src/PhotoMode/Manager.cpp cmake/sourcelist.cmake cmake/headerlist.cmake
```

Build Skyrim SE:

```bat
cmake --preset vs2022-windows-vcpkg-se
cmake --build build --config Release
```

Commit:

```bat
git add src/IGCSBridge src/Hooks.cpp src/PhotoMode/Manager.cpp cmake/sourcelist.cmake cmake/headerlist.cmake
git commit -m "Add direct IGCSDOF camera bridge"
```

## Runtime dependency

The integration remains optional at runtime. Photo Mode continues to work when
`IgcsConnector.addon64` is not loaded; the bridge simply reports that it could
not connect.

## Logging

Normal builds write only a small status file:

```text
%TEMP%\Skyrim_IGCSDOF_status.txt
```

For troubleshooting, change this constant in `src/IGCSBridge/Bridge.cpp`:

```cpp
constexpr bool kVerboseDiagnostics = false;
```

to:

```cpp
constexpr bool kVerboseDiagnostics = true;
```

This restores the detailed sample, matrix, focus-plane and connector-buffer diagnostics.
