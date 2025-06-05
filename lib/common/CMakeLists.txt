find_package(DRRAUtils REQUIRED)

# Add the common library
add_drra_folder(common)

# Copy jinja folder to output directory
message(STATUS "Copying jinja folder to output directory")
set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/common)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/jinja DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})