function(ensure_corrosion)
  if(NOT DEFINED CORROSION_FETCHED OR NOT CORROSION_FETCHED)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/modules/corrosion/CMakeLists.txt")
      message(STATUS "Fetching Corrosion")
      include(FetchContent)
      FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.5.2)
      FetchContent_MakeAvailable(Corrosion)
    else()
      message(STATUS "Using Corrosion submodule")
    endif()
    set(CORROSION_FETCHED
        TRUE
        CACHE INTERNAL "Whether Corrosion has been fetched")
  endif()

  if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/modules/corrosion/CMakeLists.txt")
    if(NOT DEFINED CORROSION_ADDED OR NOT CORROSION_ADDED)
      add_subdirectory(${CMAKE_SOURCE_DIR}/cmake/modules/corrosion
                       ${CMAKE_BINARY_DIR}/corrosion-build EXCLUDE_FROM_ALL)
      set(CORROSION_ADDED
          TRUE
          CACHE INTERNAL "Whether Corrosion has been added")
    endif()
    list(APPEND CMAKE_MODULE_PATH
         "${CMAKE_SOURCE_DIR}/cmake/modules/corrosion/cmake")
    include(Corrosion)
  endif()
endfunction()

function(cargo_build FOLDER)
  ensure_corrosion()

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_COMPONENTS_LIBRARY_DIR})

  get_filename_component(FOLDER_NAME ${FOLDER} NAME)
  message(STATUS "Found Rust project: ${FOLDER}")

  # Generate a unique hash for this package
  string(SHA256 CRATE_HASH "${FOLDER}")

  # Create temporary directory and copy the folder contents
  file(MAKE_DIRECTORY "${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}")
  file(COPY "${FOLDER}/."
       DESTINATION "${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}")

  # Copy ../isa.json to the directory
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/isa.json"
       DESTINATION "${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}")

  # Modify the Cargo.toml file DURING CMAKE CONFIGURATION
  file(READ "${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}/Cargo.toml"
       CARGO_TOML_CONTENT)
  string(REGEX
         REPLACE "(^|\n)name = \"[^\"]*\"" "\\1name = \"sha256_${CRATE_HASH}\""
                 UPDATED_CARGO_TOML_CONTENT "${CARGO_TOML_CONTENT}")
  file(WRITE "${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}/Cargo.toml"
       "${UPDATED_CARGO_TOML_CONTENT}")

  corrosion_import_crate(
    MANIFEST_PATH ${CMAKE_COMPONENTS_TEMP_DIR}/${CRATE_HASH}/Cargo.toml PROFILE
    release)

  # Get the target name created by Corrosion
  set(CORROSION_TARGET_NAME "sha256_${CRATE_HASH}")

  # Create a custom target that depends on the Corrosion target
  add_custom_target(
    copy_executable_${CRATE_HASH} ALL
    DEPENDS ${CORROSION_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${CORROSION_TARGET_NAME}>
            "${CMAKE_COMPONENTS_LIBRARY_DIR}/${FOLDER_NAME}"
    COMMAND chmod +x "${CMAKE_COMPONENTS_LIBRARY_DIR}/${FOLDER_NAME}"
    COMMENT "Copying and renaming executable to ${FOLDER_NAME}")

  add_dependencies(drra copy_executable_${CRATE_HASH})
endfunction()

function(add_drra_folder TYPE_NAME)
  # Get all subdirectories
  file(
    GLOB SUBDIRS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/*)

  # Add all subdirectories in a loop
  foreach(SUBDIR ${SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
      # Set output directory for components
      set(CMAKE_COMPONENTS_LIBRARY_DIR
          ${CMAKE_BINARY_DIR}/library/${TYPE_NAME}/${SUBDIR})

      # Add subdirectory if it has a CMakeLists.txt
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
        message(STATUS "Found component: ${SUBDIR}")
        add_subdirectory(${SUBDIR})
      endif()

      # Copy component files
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/arch.json"
         AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/isa.json")
        message(STATUS "Found component files in: ${SUBDIR}")

        # Copy all JSON files
        file(GLOB JSON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.json")
        file(COPY ${JSON_FILES} DESTINATION ${CMAKE_COMPONENTS_LIBRARY_DIR})
      endif()

      # Copy RTL folder if it exists
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl"
         AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/Bender.yml")
        message(STATUS "Found RTL folder in component: ${SUBDIR}")

        # Copy Bender file
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/Bender.yml"
             DESTINATION ${CMAKE_COMPONENTS_LIBRARY_DIR})

        # Copy RTL folder
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/rtl
             DESTINATION ${CMAKE_COMPONENTS_LIBRARY_DIR})
      endif()
    endif()
  endforeach()
endfunction()
