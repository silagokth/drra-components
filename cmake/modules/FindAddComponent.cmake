function(copy_component_files DEST_DIR)
  file(GLOB JSON_FILES "*.json")
  file(COPY ${JSON_FILES} DESTINATION ${DEST_DIR})
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtl")
    file(GLOB RTL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rtl/*.sv*)
    file(COPY ${RTL_FILES} DESTINATION ${DEST_DIR})
  endif()
endfunction()

function(compile_timing_model COMPONENT_TYPE COMPONENT_NAME CMAKE_COMPONENTS_OUTPUT_DIRECTORY)
  set(CURRENT_SUBPROJECT_NAME "${COMPONENT_TYPE}-${COMPONENT_NAME}")
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

  elseif(${COMPONENT_TYPE} STREQUAL "fabric")

    copy_component_files(${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

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
    if(EXISTS "${full_path}/sst")
      message(STATUS "Found SST component: ${subdir}")
      add_subdirectory(${full_path}/sst)
    endif()
  endforeach()
endfunction()
