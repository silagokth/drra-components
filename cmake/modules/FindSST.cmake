# FindSST.cmake - Find SST package

if(USE_SST)
  message(STATUS "USE_SST is enabled, proceeding with SST package search.")
else()
  message(STATUS "USE_SST is disabled, skipping SST package search.")
  function(sst_build)
    message(
      STATUS
      "SST build function is not called because USE_SST is disabled."
    )
  endfunction()
  return()
endif()

set(SST_PREFIX $ENV{SST_CORE_HOME})

# Find the SST executable
find_program(SST_EXECUTABLE sst
    PATHS ${SST_PREFIX}/bin
    DOC "SST executable"
)

if(NOT SST_EXECUTABLE)
  message(STATUS "SST_PREFIX: ${SST_PREFIX}")
  file(GLOB SST_BIN_FILES ${SST_PREFIX}/bin/*)
  message(STATUS "Contents of ${SST_PREFIX}/bin:")

  foreach(FILE ${SST_BIN_FILES})
    message(STATUS "  ${FILE}")
  endforeach()

  message(
    FATAL_ERROR
    "SST executable not found. Set the SST_CORE_HOME environment variable."
  )
endif()

# Cache variables
set(SST_VERSION_CACHE "" CACHE STRING "Cached SST version")
set(SST_INCLUDE_DIR_CACHE "" CACHE PATH "Cache SST include directory")
set(SST_CXX_FLAGS_CACHE "" CACHE STRING "Cache SST CXX flags")

# Get SST include directory
if(SST_EXECUTABLE AND(SST_VERSION_CACHE STREQUAL "" OR DEFINED RESET_SST_CACHE))
  message(STATUS "Fetching SST configuration")

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.10)
    # Get SST version from CLI
    execute_process(
            COMMAND ${SST_EXECUTABLE} --version
            OUTPUT_VARIABLE SST_VERSION_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ECHO NONE
        )

    # Get SST include directory
    execute_process(
            COMMAND sst-config --includedir
            OUTPUT_VARIABLE SST_INCLUDE_DIR_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ECHO NONE
        )

    # Get SST CXX flags
    execute_process(
            COMMAND sst-config --CXXFLAGS
            OUTPUT_VARIABLE SST_CXX_FLAGS_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ECHO NONE
        )
  else() # Run sequentially for older CMAKE versions
    execute_process(
            COMMAND ${SST_EXECUTABLE} --version
            OUTPUT_VARIABLE SST_VERSION_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
            COMMAND sst-config --includedir
            OUTPUT_VARIABLE SST_INCLUDE_DIR_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
            COMMAND sst-config --CXXFLAGS
            OUTPUT_VARIABLE SST_CXX_FLAGS_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
  endif()

  # Update SST cache variables
  if(SST_VERSION_RESULT AND SST_INCLUDE_DIR_RESULT AND SST_CXX_FLAGS_RESULT)
    set(
      SST_VERSION_CACHE "${SST_VERSION_RESULT}"
      CACHE STRING "Cached SST version"
      FORCE
    )
    set(
      SST_INCLUDE_DIR_CACHE "${SST_INCLUDE_DIR_RESULT}"
      CACHE PATH "Cached SST include directory"
      FORCE
    )
    set(
      SST_CXX_FLAGS_CACHE "${SST_CXX_FLAGS_RESULT}"
      CACHE STRING "Cached SST CXX flags"
      FORCE
    )
  else()
    message(
      FATAL_ERROR
      "Failed to retrieve SST configuration. Check your SST installation."
    )
  endif()

  message(STATUS "SST version: ${SST_VERSION_CACHE}")
  message(STATUS "SST include directory: ${SST_INCLUDE_DIR_CACHE}")
  message(STATUS "SST CXX flags: ${SST_CXX_FLAGS_CACHE}")
endif()

# Use the cache variables directly
set(SST_VERSION "${SST_VERSION_CACHE}")
set(SST_INCLUDE_DIR "${SST_INCLUDE_DIR_CACHE}")
set(SST_CXX_FLAGS "${SST_CXX_FLAGS_CACHE}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SST
    REQUIRED_VARS SST_EXECUTABLE SST_INCLUDE_DIR
    VERSION_VAR SST_VERSION
)

if(SST_FOUND)
  if(NOT TARGET SST::SST)
    add_library(SST::SST INTERFACE IMPORTED)
    set_target_properties(SST::SST PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SST_INCLUDE_DIR}"
            INTERFACE_COMPILE_OPTIONS "${SST_CXX_FLAGS}"
        )
  endif()

  # Set traditional variables for backward compatibility
  set(SST_INCLUDE_DIRS ${SST_INCLUDE_DIR})
endif()

mark_as_advanced(SST_EXECUTABLE SST_INCLUDE_DIR SST_CXX_FLAGS)

function(sst_build)
  if(USE_SST)
    find_package(SST REQUIRED)
    set(options)
    set(oneValueArgs OVERRIDE_SOURCE_NAME)
    set(multiValueArgs)

    cmake_parse_arguments(
            SST_BUILD
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
        )

    if(SST_BUILD_OVERRIDE_SOURCE_NAME)
      set(component_name ${SST_BUILD_OVERRIDE_SOURCE_NAME})
    else()
      cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH last_path)
      get_filename_component(component_name "${last_path}" NAME)
    endif()

    add_library(${component_name} OBJECT ${component_name}.cpp)
    set_property(TARGET ${component_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

    target_include_directories(
      ${component_name}
      PRIVATE .
      ${COMMON_INCLUDE_DIRS}
    )

    target_sources(drra PRIVATE $<TARGET_OBJECTS:${component_name}>)
  endif()
endfunction()
