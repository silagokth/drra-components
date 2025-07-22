find_package(DRRAUtils REQUIRED)

# Build timing model using Cargo
cargo_build(${CMAKE_CURRENT_SOURCE_DIR}/timing_model)
cargo_build(${CMAKE_CURRENT_SOURCE_DIR}/compile_util)

