cmake_minimum_required(VERSION 3.12)

include(/home/ashrivastava/projects/pico/pico-sdk/external/pico_sdk_import.cmake)

project(midicon C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()

add_executable(midicon
		midicon.c
		ssd1306.c	
		usb_descriptors.c
		)

target_include_directories(midicon PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(midicon
	pico_stdlib
	hardware_adc
	hardware_i2c
	tinyusb_device
	tinyusb_board
	)

# add extra ouputs for map/bin/hex files
pico_add_extra_outputs(midicon)
