function(copy_component_files DEST_DIR)
  # Copy JSON files
  file(GLOB JSON_FILES "*.json")
  file(COPY ${JSON_FILES} DESTINATION ${DEST_DIR})

  # Copy Bender file
  file(GLOB BENDER_FILE "Bender.yml")
  file(COPY ${BENDER_FILE} DESTINATION ${DEST_DIR})

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtl")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rtl DESTINATION ${DEST_DIR})
  endif()
endfunction()

function(compile_timing_model COMPONENT_TYPE COMPONENT_NAME CMAKE_COMPONENTS_OUTPUT_DIRECTORY)
  set(CURRENT_SUBPROJECT_NAME "${COMPONENT_TYPE}-${COMPONENT_NAME}")

  # Find all Rust source files (and Cargo.toml which controls dependencies)
  file(GLOB TIMING_MODEL_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/timing_model/src/*.rs"
    "${CMAKE_CURRENT_SOURCE_DIR}/timing_model/Cargo.toml"
  )

  # Create build directory using CMake commands
  set(CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo/${CURRENT_SUBPROJECT_NAME}")

  # This is the binary that cargo will produce
  set(CARGO_EXECUTABLE_PATH "${CARGO_TARGET_DIR}/release/timing_model")

  # Create output directories with CMake commands
  file(MAKE_DIRECTORY "${CARGO_TARGET_DIR}")
  file(MAKE_DIRECTORY "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}")

  # Check if sccache is available
  find_program(SCCACHE sccache)

  if(SCCACHE)
    set(CARGO_ENV_COMMAND ${CMAKE_COMMAND} -E env "RUSTC_WRAPPER=${SCCACHE}")
  else()
    set(CARGO_ENV_COMMAND ${CMAKE_COMMAND} -E env)
  endif()

  add_custom_command(
    OUTPUT "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/timing_model"
    DEPENDS ${TIMING_MODEL_SRC}

    COMMAND ${CMAKE_COMMAND} -E echo "Building ${CURRENT_SUBPROJECT_NAME} with cargo..."
    COMMAND ${CARGO_ENV_COMMAND} ${CARGO_EXECUTABLE} build --release --target-dir=${CARGO_TARGET_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy "${CARGO_EXECUTABLE_PATH}" "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/timing_model"

    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/timing_model"
    VERBATIM
  )

  add_custom_target(
    gen-${CURRENT_SUBPROJECT_NAME}
    ALL DEPENDS "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/timing_model"
  )
endfunction()

function(add_component COMPONENT_TYPE COMPONENT_NAME)
  set(CURRENT_SUBPROJECT_NAME "${COMPONENT_TYPE}-${COMPONENT_NAME}")
  set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/${COMPONENT_TYPE}/${COMPONENT_NAME})

  find_program(CARGO_EXECUTABLE cargo)
  find_program(RUSTC_EXECUTABLE rustc)

  message(STATUS "COMPONENT_TYPE: ${COMPONENT_TYPE}")

  if(${COMPONENT_TYPE} STREQUAL "resources")
    copy_component_files(${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
    compile_timing_model(${COMPONENT_TYPE} ${COMPONENT_NAME} ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

  elseif(${COMPONENT_TYPE} STREQUAL "common")
    subdirlist(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

    foreach(subdir ${SUBDIRS})
      set(subdir_path ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})

      if(IS_DIRECTORY ${subdir_path})
        copy_component_files(${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/${subdir})
      endif()
    endforeach()

  # elseif(${COMPONENT_TYPE} STREQUAL "fabric")
  # copy_component_files(${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
  else()
    copy_component_files(${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
  endif()
endfunction()

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")

  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()

  SET(${result} ${dirlist})
ENDMACRO()

function(add_all_subdirs)
  subdirlist(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

  foreach(subdir ${SUBDIRS})
    add_subdirectory(${subdir})
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})

    if(USE_SST)
      if(EXISTS "${full_path}/sst")
        message(STATUS "Found SST component: ${subdir}")
        add_subdirectory(${full_path}/sst)
      endif()
    endif()
  endforeach()
endfunction()
