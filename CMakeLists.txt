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

# Add Corrosion to build Rust components
# include(FetchContent)
# FetchContent_Declare(
# Corrosion
# GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
# GIT_TAG v0.4 # Use a specific version
# )
# FetchContent_MakeAvailable(Corrosion)

# # Add cargo sccache if installed
# find_program(SCCACHE sccache)

# if(SCCACHE)
# message(STATUS "Using sccache: ${SCCACHE}")
# set(ENV{RUSTC_WRAPPER} "${SCCACHE}")
# set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE}")
# set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE}")
# set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
# cmake_policy(SET CMP0141 NEW)
# else()
# message(STATUS "sccache not found, using default compiler")
# endif()

# SST
if(USE_SST)
  find_package(SST REQUIRED)

  if(SST_FOUND AND NOT SST_PREFIX)
    # Try common locations
    if(DEFINED ENV{SST_CORE_HOME})
      set(SST_PREFIX $ENV{SST_CORE_HOME})
      message(STATUS "Using SST_PREFIX from SST_CORE_HOME: ${SST_PREFIX}")
    elseif(DEFINED SST_DIR)
      get_filename_component(SST_PREFIX "${SST_DIR}" DIRECTORY DIRECTORY)
      message(STATUS "Derived SST_PREFIX from SST_DIR: ${SST_PREFIX}")
    else

      execute_process(
        COMMAND which sst
        OUTPUT_VARIABLE SST_BIN
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if(SST_BIN)
        get_filename_component(SST_PREFIX "${SST_BIN}" DIRECTORY DIRECTORY)
        message(STATUS "Derived SST_PREFIX from sst binary: ${SST_PREFIX}")
      endif()
    endif()
  endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/common/sst
  )
  target_include_directories(drra PUBLIC ${COMMON_INCLUDE_DIRS})
  target_link_libraries(drra PRIVATE timingModel)

  add_subdirectory(sst_tests)

  # Register DRRA library and its components
  add_custom_command(TARGET drra POST_BUILD
    COMMAND echo "Registering SST library: drra"

    # COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:drra> ${CMAKE_CURRENT_BINARY_DIR}/library

    # COMMAND sst-register drra drra_LIBDIR=${CMAKE_CURRENT_BINARY_DIR}/library
    # COMMAND sst-register SST_ELEMENT_TESTS drra=${CMAKE_CURRENT_SOURCE_DIR}/sst_tests
  )
endif()

# The project version number.
set(VERSION_MAJOR 1 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

add_subdirectory("lib")