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
    COMMAND ${PYTHON3} -c
        "import json, pathlib; \
         root = pathlib.Path('${CMAKE_SOURCE_DIR}'); \
         dbs = list(root.glob('*/build/compile_commands.json')); \
         merged = sum((json.loads(p.read_text()) for p in dbs), []); \
         (root / 'compile_commands.json').write_text(json.dumps(merged, indent=2)); \
         print(f'gen_compile_commands: merged {len(merged)} entries from {len(dbs)} databases')"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Merging compile_commands.json from subprojects"
)
