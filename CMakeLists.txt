cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(SmileStick)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

if (TARGET tinyusb_device)

    add_executable(SmileStick
        main.c
        core2.c
    )

    # Add pico_stdlib library which aggregates commonly used features
    target_link_libraries(SmileStick 
        pico_multicore 
        pico_stdlib
    )

    # enable usb output, disable uart output
    pico_enable_stdio_usb(SmileStick 1)
    pico_enable_stdio_uart(SmileStick 0)

    # create map/bin/hex/uf2 file in addition to ELF.
    pico_add_extra_outputs(SmileStick)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
