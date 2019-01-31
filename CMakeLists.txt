cmake_minimum_required(VERSION 2.8)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "-O0 -ggdb3 ${CMAKE_C_FLAGS}" )
    set(CMAKE_CXX_FLAGS "-O0 -ggdb3 ${CMAKE_CXX_FLAGS}")
endif()

set (CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

set(TARGET aec_test)

project(${TARGET})

set(WEBRTCAP_INC_DIR "/usr/include/webrtc_audio_processing/")
set(WEBRTCAP_INC_ALSA "/usr/include/alsa/")

set(SOURCES
    "main.cpp"
    "alsa_device.cpp"
    "aec_controller.cpp"
    )

set(HEADERS
    "alsa_device.h"
    "aec_controller.h"
    )

include_directories(
                    ${WEBRTCAP_INC_DIR}
                    )


add_executable(${TARGET}
               ${SOURCES}
               ${HEADERS}
                )

target_link_libraries(${TARGET}
                        webrtc_audio_processing
                        asound
                        )
