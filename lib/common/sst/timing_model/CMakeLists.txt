cmake_minimum_required(VERSION 3.10)

project(drra_timing_model)

file(GLOB SOURCES src/*.cpp src/*.c)
add_library(drra_timing_model STATIC ${SOURCES})
set_property(TARGET drra_timing_model PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(drra_timing_model PRIVATE ./include/)
