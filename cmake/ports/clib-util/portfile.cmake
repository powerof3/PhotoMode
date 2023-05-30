# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF bb05775e3d0fb0e995e22d99ff84b8d6e6443ba8
    SHA512 c858e65c1817c41271bc65b0ae4740eba16c3cc2c9edb2583f121ec93ecd3b9c67e9b3922af9f9141de59693b840a163e7ec1e1048a4fe8ba4a03bf64db449f4
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
