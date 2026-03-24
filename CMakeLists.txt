cmake_minimum_required(VERSION 3.16)
project(XorOS LANGUAGES CXX)

add_subdirectory(rv32i)

include(ExternalProject)
ExternalProject_Add(xoros_kernel
    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/os
    BINARY_DIR  ${CMAKE_SOURCE_DIR}/os/build
    CMAKE_ARGS  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DSIMULATOR=$<TARGET_FILE:rv32i_cpu>
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
    DEPENDS rv32i_cpu
)

find_program(PYTHON3 python3 REQUIRED)
add_custom_target(gen_compile_commands
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/merge_compile_commands.py
            ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Merging compile_commands.json from subprojects"
)
