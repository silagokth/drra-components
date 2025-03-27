find_package(AddComponent REQUIRED)

add_subdirectory(cells)
add_subdirectory(common)
add_subdirectory(controllers)
add_subdirectory(fabric)
add_subdirectory(resources)

set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tb DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})