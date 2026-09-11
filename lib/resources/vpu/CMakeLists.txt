find_package(DRRAUtils REQUIRED)

add_subdirectory(sst)

# Build timing model using Cargo
cargo_build(${CMAKE_CURRENT_SOURCE_DIR}/compile_util)
