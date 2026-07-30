# IGCSDOF V20 changeset — technical notes

## 1. New direct bridge

### `src/IGCSBridge/Bridge.h`
Declares the singleton responsible for:

- discovering `IgcsConnector.addon64` dynamically;
- publishing camera position, FOV, quaternion, view matrix, and camera axes;
- receiving the camera commands exported by IGCSDOF;
- saving and restoring the camera at the beginning and end of a screenshot session;
- applying multishot and panorama camera movement.

### `src/IGCSBridge/Bridge.cpp`
Implements the bridge and exports the ABI expected by IGCSDOF:

```cpp
IGCS_StartScreenshotSession
IGCS_EndScreenshotSession
IGCS_MoveCameraMultishot
IGCS_MoveCameraPanorama
```

The key V20 correction is in `BuildBasis()`: the camera basis is reconstructed
using Photo Mode's native rotation convention, equivalent to
`FromEulerAnglesZXY`. The matrix columns are interpreted as:

```text
Right / Forward / Up
```

Camera rotation remains fixed for every aperture sample. Only translation is
changed, inside the plane formed by `Right` and `Up`.

## 2. Rendered camera hook

### `src/Hooks.cpp`
Adds a vtable hook on `RE::FreeCameraState::GetTranslation`.

Flow:

```text
Skyrim calculates the native translation
→ the bridge receives the native value
→ IGCSDOF overrides only the rendered translation during a session
→ native rotation and FOV remain untouched
```

Photo Mode's existing `FromEulerAnglesZXY` hook is preserved and remains the
source of truth for the rotation convention.

## 3. Photo Mode lifecycle integration

### `src/PhotoMode/Manager.cpp`
Three bridge calls are added:

- activation: connect to IgcsConnector and publish the initial camera state;
- frame update: continuously publish the current camera state;
- deactivation: end any active screenshot session, restore the camera, and clear the connector buffer.

The bridge is therefore available only while Photo Mode is actually active.

## 4. CMake integration

### `cmake/sourcelist.cmake`
Adds:

```cmake
src/IGCSBridge/Bridge.cpp
```

### `cmake/headerlist.cmake`
Adds:

```cmake
src/IGCSBridge/Bridge.h
```

## 5. Logging

The GitHub-ready version writes a minimal status file to:

```text
%TEMP%\Skyrim_IGCSDOF_status.txt
```

Detailed diagnostics remain available but are disabled by default through:

```cpp
constexpr bool kVerboseDiagnostics = false;
```

This allows full sample, matrix, focus-plane, and connector-buffer diagnostics
to be restored without reintroducing the old test code.

## 6. Validation

V20 was validated across multiple renders with:

- exact sample translation application;
- no rotation drift;
- no FOV drift;
- an orthonormal native ZXY camera basis;
- visually stable focus in steep top-down shots and several other camera orientations.

## 7. Not included in this commit

Automatic Photo Mode UI resizing during hotsampling should be handled in a
separate commit. It is independent from the camera bridge and will be easier to
review or submit as a separate pull request.
