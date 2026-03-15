cmake_minimum_required(VERSION 3.16)

project(csound_package_test C)

find_package(Csound CONFIG REQUIRED)

message(STATUS "Using Csound package from: ${Csound_DIR}")
message(STATUS "Using Csound target: Csound::Csound")

add_executable(csound_package_test main.c)
target_link_libraries(csound_package_test PRIVATE Csound::Csound)
