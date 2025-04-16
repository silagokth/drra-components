cmake_minimum_required(VERSION 3.5)
project(drra-component-lib)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC OFF)

# Create a USE_SST option (default ON)
option(USE_SST "Compile SST components, needed for SST simulation" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# SST
if(USE_SST)
  # Make sure module path is set (already done in your file)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

  # Find SST package
  find_package(SST REQUIRED)

  # Add DRRA SST shared library
  add_library(drra SHARED)

  set(COMMON_INCLUDE_DIRS
    ${SST_INCLUDE_DIRS}
    $ENV{SST_ELEMENTS_HOME}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/common/sst
  )

  target_include_directories(drra PUBLIC ${COMMON_INCLUDE_DIRS})
  target_link_libraries(drra PRIVATE timingModel)

  # If you're using the modern CMake approach with SST::SST target:
  # target_link_libraries(drra PRIVATE timingModel SST::SST)
  add_subdirectory(sst_tests)

  # Register DRRA library and its components
  add_custom_command(TARGET drra POST_BUILD
    COMMAND echo "Registering SST library: drra"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:drra> ${CMAKE_CURRENT_BINARY_DIR}/library
  )
endif()

# The project version number.
set(VERSION_MAJOR 1 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

add_subdirectory("lib")