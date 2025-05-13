function(cargo_build FOLDER)
    include(FetchContent)
    FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.5.2
    )
    FetchContent_MakeAvailable(Corrosion)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

    get_filename_component(FOLDER_NAME ${FOLDER} NAME)
    message(STATUS "FOLDER_NAME: ${FOLDER_NAME}")

    # Create temporary directory
    file(MAKE_DIRECTORY "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp")
    file(COPY "${FOLDER}" DESTINATION "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp")

    # Generate a unique hash for this package
    string(SHA256 CRATE_HASH "${FOLDER}")

    # Modify the Cargo.toml file DURING CMAKE CONFIGURATION
    file(
        READ "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp/${FOLDER_NAME}/Cargo.toml"
        CARGO_TOML_CONTENT
    )

    # Replace the package name in Cargo.toml
    string(REGEX REPLACE "(^|\n)name = \"[^\"]*\"" "\\1name = \"sha256_${CRATE_HASH}\"" UPDATED_CARGO_TOML_CONTENT "${CARGO_TOML_CONTENT}")

    # Write the updated content back to Cargo.toml
    file(WRITE "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp/${FOLDER_NAME}/Cargo.toml" "${UPDATED_CARGO_TOML_CONTENT}")

    corrosion_import_crate(
        MANIFEST_PATH ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp/${FOLDER_NAME}/Cargo.toml
        PROFILE release
    )

    # Get the target name created by Corrosion
    set(CORROSION_TARGET_NAME "sha256_${CRATE_HASH}")

    # Create a custom target that depends on the Corrosion target
    add_custom_target(
        copy_${CRATE_HASH} ALL
        DEPENDS ${CORROSION_TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${CORROSION_TARGET_NAME}>
        "${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/${FOLDER_NAME}"
        COMMENT "Copying and renaming executable to ${FOLDER_NAME}"
    )

    # Clean up temp directory at build time
    add_custom_target(
        cleanup_${CRATE_HASH} ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY}/temp
        COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:${CORROSION_TARGET_NAME}>
        COMMENT "Cleaning up temporary build directory"
    )
endfunction()

function(add_drra_folder TYPE_NAME)
    # Get all subdirectories
    file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

    # Add all subdirectories in a loop
    foreach(SUBDIR ${SUBDIRS})
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
            # Set output directory for components
            set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/${TYPE_NAME}/${SUBDIR})

            # Add subdirectory if it has a CMakeLists.txt
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
                message(STATUS "Found component: ${SUBDIR}")
                add_subdirectory(${SUBDIR})
            endif()

            # Copy component files
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/arch.json" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/isa.json")
                message(STATUS "Found component files in: ${SUBDIR}")

                # Copy all JSON files
                file(GLOB JSON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.json")
                file(COPY ${JSON_FILES} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
            endif()

            # Copy RTL folder if it exists
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/Bender.yml")
                message(STATUS "Found RTL folder in component: ${SUBDIR}")

                # Copy Bender file
                file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/Bender.yml" DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

                # Copy RTL folder
                file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})
            endif()
        endif()
    endforeach()
endfunction()