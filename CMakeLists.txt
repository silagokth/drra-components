cmake_minimum_required(VERSION 3.5)
project(drra-component-lib)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SST package
find_package(SST REQUIRED)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# Set SST include directories
execute_process(
  COMMAND sst-config --includedir
  OUTPUT_VARIABLE SST_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add DRRA SST shared library
add_library(drra SHARED)
set(COMMON_INCLUDE_DIRS
  ${SST_INCLUDE_DIR}
  $ENV{SST_ELEMENTS_HOME}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/utils/sst
)
target_include_directories(drra PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(drra PRIVATE timingModel)

add_subdirectory(testcases)

# The project version number.
set(VERSION_MAJOR 1 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

add_subdirectory("lib")

# Register DRRA library and its components
add_custom_command(TARGET drra POST_BUILD
  COMMAND echo "Registering SST library: drra"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:drra> ${CMAKE_CURRENT_BINARY_DIR}/lib
  COMMAND sst-register drra drra_LIBDIR=${CMAKE_CURRENT_BINARY_DIR}/lib
  COMMAND sst-register SST_ELEMENT_TESTS drra=${CMAKE_CURRENT_SOURCE_DIR}/testcases/sst
)
