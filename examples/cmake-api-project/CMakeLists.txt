cmake_minimum_required(VERSION 3.16)

project(csound_package_test C CXX)

option(USE_STATIC "Link against static Csound library" OFF)

find_package(Csound CONFIG REQUIRED)

if(USE_STATIC)
    set(_csound_target Csound::Csound-static)
else()
    set(_csound_target Csound::Csound)
endif()

message(STATUS "Using Csound package from: ${Csound_DIR}")
message(STATUS "Using Csound target: ${_csound_target}")

add_executable(csound_package_test main.c)
target_link_libraries(csound_package_test PRIVATE ${_csound_target})
