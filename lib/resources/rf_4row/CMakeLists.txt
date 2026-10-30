find_package(DRRAUtils REQUIRED)

add_subdirectory(sst)

# Build the compile_util binary (lib/common/compile_util unless this component
# ships its own compile_util/ folder)
drra_compile_util()
