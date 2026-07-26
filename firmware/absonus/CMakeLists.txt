# absonus firmware — standalone libDaisy + DaisySP CMake project.
#
# Configure + build:  cmake -B build && cmake --build build   (from this directory)
# Flash (Daisy in DFU): cmake --build build --target program-dfu

cmake_minimum_required(VERSION 3.26)

set(FIRMWARE_NAME absonus)
set(FIRMWARE_SOURCES
    absonus.cpp
    fmchorus.cpp
    cubicnl.cpp
    tremor.cpp
    port.cpp
    reverbsc.cpp
)

set(LIBDAISY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libDaisy)
set(DAISYSP_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../DaisySP)

list(APPEND CMAKE_MODULE_PATH ${LIBDAISY_DIR}/cmake)
include(${LIBDAISY_DIR}/cmake/toolchains/autodetect.cmake)

# Generate a raw .bin for DFU flashing (needed by the program-dfu target below).
set(DAISY_GENERATE_BIN ON CACHE BOOL "" FORCE)

project(${FIRMWARE_NAME} C CXX ASM)

include(DaisyDefaultBuild)

# newlib-nano omits float support in printf/vsnprintf by default; libDaisy's
# Logger uses vsnprintf, so pull it in to keep %f working in the kDebug output.
target_link_options(${FIRMWARE_NAME} PRIVATE LINKER:-u,_printf_float)

# Flash over USB DFU (hold BOOT, tap RESET first). No extra hardware needed.
add_custom_target(program-dfu
    COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${FIRMWARE_NAME}.bin -d ,0483:df11
    DEPENDS ${FIRMWARE_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Flashing ${FIRMWARE_NAME}.bin over DFU")

# Flash over SWD with an ST-Link (OpenOCD). No BOOT+RESET dance; also the path
# used for live debugging (see firmware/.vscode/launch.json).
add_custom_target(program
    COMMAND openocd -f interface/stlink.cfg -f target/stm32h7x.cfg
            -c "program ${FIRMWARE_NAME}.elf verify reset exit"
    DEPENDS ${FIRMWARE_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Flashing ${FIRMWARE_NAME}.elf via ST-Link (OpenOCD)")