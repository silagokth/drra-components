find_package(DRRAUtils REQUIRED)

# Descend into the two halves of the common library: rtl/ (Bender packages,
# copied to the output tree) and sst/ (C++ compiled into libdrra.so).
# compile_util/ and jinja/ have no CMakeLists.txt and belong to neither --
# compile_util is built per-resource by drra_compile_util(), and jinja is
# copied wholesale below.
add_drra_folder(common)

# Copy jinja folder to output directory
message(STATUS "Copying jinja folder to output directory")
set(CMAKE_COMPONENTS_LIBRARY_DIR ${CMAKE_BINARY_DIR}/library/common)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/jinja
     DESTINATION ${CMAKE_COMPONENTS_LIBRARY_DIR})
