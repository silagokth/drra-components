find_package(DRRAUtils REQUIRED)

add_subdirectory(cells)
add_subdirectory(common)
add_subdirectory(controllers)
add_subdirectory(fabric)
add_subdirectory(resources)

set(CMAKE_COMPONENTS_LIBRARY_DIR ${CMAKE_BINARY_DIR}/library)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tb DESTINATION ${CMAKE_COMPONENTS_LIBRARY_DIR})
