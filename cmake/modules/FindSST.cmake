# FindSST.cmake - Find SST package

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

    message(FATAL_ERROR "SST executable not found. Please set the SST_CORE_HOME environment variable.")
endif()

# Get SST include directory
if(SST_EXECUTABLE)
    execute_process(
        COMMAND ${SST_EXECUTABLE} --version
        OUTPUT_VARIABLE SST_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND sst-config --includedir
        OUTPUT_VARIABLE SST_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Also grab CXXFLAGS which might contain important compile definitions
    execute_process(
        COMMAND sst-config --CXXFLAGS
        OUTPUT_VARIABLE SST_CXX_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

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

        target_include_directories(${component_name} PRIVATE . ${COMMON_INCLUDE_DIRS})

        target_sources(drra PRIVATE $<TARGET_OBJECTS:${component_name}>)
    endif()
endfunction(sst_build)