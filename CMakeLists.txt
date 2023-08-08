cmake_minimum_required(VERSION 3.16)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(BoardFirmware VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Create a pico-sdk subdirectory in our projecty for the libraries
pico_sdk_init()

add_executable(${CMAKE_PROJECT_NAME} main.cpp
                                     configuration.h)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(${CMAKE_PROJECT_NAME} pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})

# Link to pico_std_lib (gpio, time, etc. functions)
target_link_libraries(${CMAKE_PROJECT_NAME} 
                      pico_stdlib
                      hardware_pio
                      hardware_pwm  # Not in use
                      hardware_i2c
                      pico_multicore
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
                           "${PROJECT_BINARY_DIR}"
                           "${PROJECT_SOURCE_DIR}/BoardFirmware/src"
                           "${PROJECT_SOURCE_DIR}/BoardFirmware/include"
                           "${PROJECT_SOURCE_DIR}/BoardFirmware/libs"
)

# enable usb output, disable uart output
pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 0)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 1)
