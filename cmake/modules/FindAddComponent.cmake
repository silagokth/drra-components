function(add_component COMPONENT_TYPE COMPONENT_NAME)

  set(CURRENT_SUBPROJECT_NAME "${COMPONENT_TYPE}-${COMPONENT_NAME}")
  set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/${COMPONENT_TYPE}/${COMPONENT_NAME})

  find_program(CARGO_EXECUTABLE cargo)
  find_program(RUSTC_EXECUTABLE rustc)

  message(STATUS "COMPONENT_TYPE: ${COMPONENT_TYPE}")

  if(${COMPONENT_TYPE} STREQUAL "resources")

    set(FILE_LIST arch.json isa.json rtl.sv)
    file(COPY ${FILE_LIST} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

    # timing_model
    file(GLOB TIMING_MODEL_SRC "timing_model/src/*.rs")
    add_custom_command(
      OUTPUT ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/timing_model
      DEPENDS ${TIMING_MODEL_SRC}
      COMMAND mkdir -p ${CMAKE_BINARY_DIR}/cargo/${CURRENT_SUBPROJECT_NAME} &&
      cargo build --release --target-dir=${CMAKE_BINARY_DIR}/cargo/${CURRENT_SUBPROJECT_NAME} &&
      cp -p ${CMAKE_BINARY_DIR}/cargo/${CURRENT_SUBPROJECT_NAME}/release/timing_model ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/timing_model
    )
    add_custom_target(gen-${CURRENT_SUBPROJECT_NAME} ALL DEPENDS ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/timing_model)

  elseif(${COMPONENT_TYPE} STREQUAL "controllers")

    set(FILE_LIST arch.json isa.json rtl.sv)
    file(COPY ${FILE_LIST} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})


  elseif(${COMPONENT_TYPE} STREQUAL "cells")

    set(FILE_LIST arch.json isa.json rtl.sv)
    file(COPY ${FILE_LIST} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

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
    if(EXISTS "${full_path}/sst")
      message(STATUS "Found SST component: ${subdir}")
      add_subdirectory(${full_path}/sst)
    endif()
  endforeach()
endfunction()
