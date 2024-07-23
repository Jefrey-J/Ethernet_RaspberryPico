cmake_minimum_required(VERSION 3.12)

# Import the Pico SDK
include(pico_sdk_import.cmake)

project(pico_tcp_ip)

# Initialize the Pico SDK
pico_sdk_init()

add_executable(pico_tcp_ip
    main.c
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(pico_tcp_ip 1)
pico_enable_stdio_uart(pico_tcp_ip 0)

# Link the necessary libraries
target_link_libraries(pico_tcp_ip hardware_pio hardware_dma pico_stdlib pico_unique_id rmii_phy)

# Create map/bin/hex/uf2 file in addition to ELF
pico_add_extra_outputs(pico_tcp_ip)

add_subdirectory(rmii_phy)
