cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_C_COMPILER_WORKS 1)

project(xoros LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ARCH_FLAGS -march=rv32ia_zicsr -mabi=ilp32)

add_compile_options(
    ${ARCH_FLAGS}
    -nostdlib -ffreestanding
    -Wall -Wextra -Werror
)

add_executable(xoros src/boot.S src/kernel_main.c src/trap.S src/trap.c src/sched_switch.S src/scheduler.c src/vmem.c src/pipe.c src/uart.c src/kalloc.c)
target_include_directories(xoros PUBLIC include)
target_link_options(xoros PRIVATE
    ${ARCH_FLAGS}
    -nostdlib -ffreestanding
    -Ttext=0x0 -e _start
)

# flat binary for the simulator
add_custom_command(TARGET xoros POST_BUILD
    COMMAND riscv64-unknown-elf-objcopy -O binary xoros xoros.bin
    COMMENT "Creating xoros.bin"
)

set(SIMULATOR "${CMAKE_CURRENT_SOURCE_DIR}/../build/rv32i/rv32i_cpu" CACHE FILEPATH "Path to rv32i_cpu simulator" FORCE)

add_executable(xoros_tests src/boot.S tests/test_ecall.c src/trap.S src/trap.c src/sched_switch.S src/scheduler.c src/vmem.c src/pipe.c src/kalloc.c)
target_include_directories(xoros_tests PUBLIC include)
target_link_options(xoros_tests PRIVATE
    ${ARCH_FLAGS}
    -nostdlib -ffreestanding
    -Ttext=0x0 -e _start
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${ARCH_FLAGS} -print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
target_link_libraries(xoros ${LIBGCC_PATH})
target_link_libraries(xoros_tests ${LIBGCC_PATH})
add_custom_command(TARGET xoros_tests POST_BUILD
    COMMAND riscv64-unknown-elf-objcopy -O binary xoros_tests xoros_tests.bin
    COMMENT "Creating xoros_tests.bin"
)
add_custom_target(run_tests
    COMMAND ${SIMULATOR} ${CMAKE_CURRENT_BINARY_DIR}/xoros_tests.bin
    DEPENDS xoros_tests
    COMMENT "Running OS tests on simulator"
)
