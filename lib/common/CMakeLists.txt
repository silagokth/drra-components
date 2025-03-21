# Get all subdirectories
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Add all subdirectories in a loop
foreach(SUBDIR ${SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR} AND NOT SUBDIR STREQUAL "sst")
        add_subdirectory(${SUBDIR})
    endif()
endforeach()
