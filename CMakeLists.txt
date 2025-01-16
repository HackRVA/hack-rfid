cmake_minimum_required(VERSION 3.28)

if(BUILD_FOR_LINUX)
else()
  set(PICO_SDK_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK from Git. Default on.")
  set(PICO_EXTRAS_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK Extras from Git. Default on.")

  include(pico_sdk_import.cmake)
  include(pico_extras_import.cmake)
  pico_sdk_init()
endif()

project(hack_rfid LANGUAGES C CXX)

option(BUILD_FOR_LINUX "Build for Linux instead of Pico" OFF)

include(FetchContent)

FetchContent_Declare(
  tiny-json
  GIT_REPOSITORY https://github.com/rafagafe/tiny-json.git
  GIT_TAG master
)

FetchContent_MakeAvailable(tiny-json)

if(BUILD_FOR_LINUX)
    message(STATUS "Building for Linux")
    add_compile_options(
      -Wno-format
      -Wno-unused-function
      -Wno-maybe-uninitialized
    )

    add_executable(hack_rfid
        src/main.c
        src/sys/sys.c
        src/acl.c
        src/sys/fs_sim.c
        ${tiny-json_SOURCE_DIR}/tiny-json.c
    )

    target_include_directories(hack_rfid PRIVATE
      ${tiny-json_SOURCE_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/src/sys/
    )


else()
  # FetchContent_Declare(
  #   littlefs
  #   GIT_REPOSITORY https://github.com/littlefs-project/littlefs.git
  #   GIT_TAG v2.10.1
  # ) 
  #
  # FetchContent_Declare(
  #   littlefs-pico
  #   GIT_REPOSITORY https://github.com/oyama/pico-littlefs-usb.git
  #   GIT_TAG main
  # )
  #
  # FetchContent_MakeAvailable(littlefs littlefs-pico)

    message(STATUS "Building for Raspberry Pi Pico")

    add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)
    add_compile_options(
      -Wno-format
      -Wno-unused-function
      -Wno-maybe-uninitialized
    )

    add_executable(hack_rfid
      src/main.c
      src/sys/sys.c
      src/acl.c
      src/sys/rfid_reader.c 
      src/sys/device/mfrc522.c 
      src/sys/device/relay.c 
      ${tiny-json_SOURCE_DIR}/tiny-json.c
      # ${littlefs_SOURCE_DIR}/lfs.c
      # ${littlefs_SOURCE_DIR}/lfs_util.c
      # ${littlefs-pico_SOURCE_DIR}/littlefs_driver.c
    )

    target_compile_definitions(hack_rfid PRIVATE __PICO_BUILD__)

    target_include_directories(hack_rfid PRIVATE
      ${tiny-json_SOURCE_DIR}
      # ${littlefs_SOURCE_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/src/sys/
    )

    pico_enable_stdio_usb(hack_rfid 1)
    pico_enable_stdio_uart(hack_rfid 0)

    target_link_libraries(hack_rfid PRIVATE
      pico_cyw43_arch_lwip_threadsafe_background
      pico_stdlib
      pico_multicore
      hardware_spi
    )

    set_target_properties(hack_rfid PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    pico_add_extra_outputs(hack_rfid)
endif()
