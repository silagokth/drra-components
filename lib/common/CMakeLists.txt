# Get all subdirectories
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Add all subdirectories in a loop
foreach(SUBDIR ${SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        if(SUBDIR STREQUAL "sst" AND USE_SST)
            add_subdirectory(sst)
        endif()

        if(SUBDIR STREQUAL "io_buffer" AND USE_SST)
            add_subdirectory(io_buffer)
        endif()

        if(NOT SUBDIR STREQUAL "sst" AND NOT SUBDIR STREQUAL "io_buffer")
            # Add subdirectory if it has a CMakeLists.txt
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
                add_subdirectory(${SUBDIR})
            endif()

            # Copy RTL folder if it exists
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl")
                set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/common/${SUBDIR})

                # Copy Bender file
                file(GLOB BENDER_FILE "Bender.yml")
                file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/${BENDER_FILE} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

                file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
            endif()
        endif()
    endif()
endforeach()
