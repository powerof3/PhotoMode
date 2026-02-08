# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF 789abd27b8955e974da53713898bf8a3db0e2a66
    SHA512 0a0a674bdcc6bfbcb2bf9bfd089b46d6826715831ce4dd51c8f89e8d3971ecb3d53d1b7027513db094b7bcb3c3420820dfabfb960e111bcaa1a5060cd9c30c0b
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
