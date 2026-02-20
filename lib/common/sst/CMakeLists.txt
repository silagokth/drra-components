cmake_minimum_required(VERSION 3.10)

if (USE_SST)
  message(STATUS "SST support enabled")
else()
  message(STATUS "SST support disabled")
  return()
endif()

# Set the project name
project(drra_sst_utils)

# Find SST package
execute_process(
    COMMAND sst-config --includedir
    OUTPUT_VARIABLE SST_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT SST_INCLUDE_DIR)
  message(FATAL_ERROR "SST not found. Please ensure SST is installed and sst-config is in your PATH.")
endif()

# Add the executable
file(GLOB SOURCES src/*.cpp src/*.c)
add_library(drra_sst_utils STATIC ${SOURCES})
set_property(TARGET drra_sst_utils PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(drra_sst_utils PRIVATE ./include/ ${SST_INCLUDE_DIR})
