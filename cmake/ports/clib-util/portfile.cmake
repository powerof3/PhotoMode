# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF ec934d91e0b9041aea7d34ceeceafdf592353909
    SHA512 c51b3e1b1e4908af8da66fa62d17ba9900f1b98e32e20a86d08ecd303a5533b978c3233aacff78e4d7bd7e22f68bd0134788ac06c46efdee27ecefc8b8e238c3
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
