vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
OUT_SOURCE_PATH SOURCE_PATH
REPO ocornut/imgui
REF db26fe7ca8de76d7802cb5c74f683e42870754ee
SHA512 a81081928d01e8f9a19815a0c0b75856c1bc08ceb0eea849dff7faf17266285c184b862a82f78335283b6cebbd29ba21de38a9caca9b4ba1a8c383589b2d97f1
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
