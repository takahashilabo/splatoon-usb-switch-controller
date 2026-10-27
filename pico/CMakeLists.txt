cmake_minimum_required(VERSION 3.13)

# pico_sdk_import.cmake is NOT included in this repo.
# Copy it from your Pico SDK checkout before building:
#   cp $PICO_SDK_PATH/external/pico_sdk_import.cmake .
include(pico_sdk_import.cmake)

project(splatoon_procon C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(splatoon_procon
    src/main.c
    src/usb_descriptors.c
    src/hori_gamepad.c
    src/splatoon_ai.c
)

target_include_directories(splatoon_procon PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(splatoon_procon PRIVATE
    pico_stdlib
    pico_rand
    tinyusb_device
)

pico_add_extra_outputs(splatoon_procon)
