cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH last_path)
get_filename_component(component_name "${last_path}" NAME)
add_library(${component_name} OBJECT ${component_name}.cpp)
set_property(TARGET ${component_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(${component_name} PRIVATE . ${COMMON_INCLUDE_DIRS})

target_sources(drra PRIVATE $<TARGET_OBJECTS:${component_name}>)
