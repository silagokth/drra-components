set(CMAKE_COMPONENTS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/utils)

set(FILE_LIST agu.sv sram:sim.sv sram:tsmc28.sv)
file(COPY ${FILE_LIST} DESTINATION ${CMAKE_COMPONENTS_OUTPUT_DIRECTORY})

#add_subdirectory(sst)
