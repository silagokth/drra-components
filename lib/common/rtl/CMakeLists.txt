find_package(DRRAUtils REQUIRED)

# Shared RTL modules. Each is a Bender package copied to
# ${CMAKE_BINARY_DIR}/library/common/rtl/<name>; resources depend on them by
# relative path, which resolves the same way in the source tree and in the
# installed library.
add_drra_folder(common/rtl)
