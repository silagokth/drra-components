cmake_minimum_required(VERSION 3.10)

add_subdirectory(timing_model)

# Set the project name
project(drra_sst_utils)

# Add the executable
file(GLOB SOURCES src/*.cpp src/*.c)
add_library(drra_sst_utils STATIC ${SOURCES})
set_property(TARGET drra_sst_utils PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(drra_sst_utils PRIVATE ./include/)
target_link_libraries(drra_sst_utils PRIVATE drra_timing_model)
