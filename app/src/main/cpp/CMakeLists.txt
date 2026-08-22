cmake_minimum_required(VERSION 3.22.1)
project("frequency_panner")

add_library(frequency_panner SHARED
    frequency_panner.cpp
)

target_link_libraries(frequency_panner
    android
    log
)
