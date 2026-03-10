cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_app C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME}
  main.c
  src/bme280.c
  src/pico_internal.c
  src/pico_rtc.c
  src/lcd_i2c.c
  src/moisture_sensor.c
)

target_include_directories(${PROJECT_NAME} PUBLIC include)

# Required for Pico
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  pico_util  # Added for datetime_to_str utility
  hardware_i2c
  hardware_adc
  hardware_rtc
  #pico_cyw43_arch_none  # Required for Pico W
)

# Enable USB and UART output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Generate the compile_commands.json file for Neovim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_add_extra_outputs(${PROJECT_NAME})

# --- Flash Raspberry Pi Pico for OpenOCD ---

# Define the debugger interface
set(DEBUG_INTERFACE "interface/cmsis-dap.cfg"
    CACHE STRING "OpenOCD interface config"
)
set(DEBUG_TARGET "target/rp2040.cfg"
    CACHE STRING "OpenOCD target config"
)

# Look for openocd in the system PATH
find_program(OPENOCD_BIN openocd)

# 2. Add the 'flash' target
if(OPENOCD_BIN)
  add_custom_target(flash
    # This command runs OpenOCD with the necessary flags
    COMMAND ${OPENOCD_BIN} -f ${DEBUG_INTERFACE} -f ${DEBUG_TARGET}
            -c "adapter speed 5000"
            -c "program ${PROJECT_NAME}.elf verify reset exit"

    # 3. Make sure the code is built before we try to flash it
    DEPENDS ${PROJECT_NAME}

    # Optional: print a message
    COMMENT "Flashing ${PROJECT_NAME}.elf to the Pico via OpenOCD..."
    VERBATIM
  )
else()
  message(WARNING "OpenOCD not found. 'flash' target will not be available.")
endif()
