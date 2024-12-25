vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
OUT_SOURCE_PATH SOURCE_PATH
REPO ocornut/imgui
REF 6982ce43f5b143c5dce5fab0ce07dd4867b705ae
SHA512 8e88b1c42ec226cce59f67a67f6189f45d49f3344082839716360f05951db286c55d4093d246ba3ffef948e1423cbebecc4d2f6dfa2106a5e4127565160acd57
HEAD_REF master
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/imgui-config.cmake.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES 
    dx11-binding                IMGUI_BUILD_DX11_BINDING
    win32-binding               IMGUI_BUILD_WIN32_BINDING
    freetype                    IMGUI_FREETYPE
    wchar32                     IMGUI_USE_WCHAR32
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
    OPTIONS_DEBUG
        -DIMGUI_SKIP_HEADERS=ON
)

vcpkg_cmake_install()

if ("freetype" IN_LIST FEATURES)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/imconfig.h" "//#define IMGUI_ENABLE_FREETYPE" "#define IMGUI_ENABLE_FREETYPE")
endif()
if ("wchar32" IN_LIST FEATURES)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/imconfig.h" "//#define IMGUI_USE_WCHAR32" "#define IMGUI_USE_WCHAR32")
endif()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
