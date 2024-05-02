cmake_minimum_required(VERSION 3.20.2)
project(meeting_sdk_linux_sample VERSION 1.0.1)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(ZOOM_SDK lib/zoomsdk)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

#This is include directory for samplerate.h
include_directories(/usr/include)

#include_directories(/home/user/Downloads/curls/curl-8.4.0/include)
include_directories(/usr/local/include)


#link_directories("/home/user/Downloads/curls/curl-8.4.0/build/lib")
link_directories(/usr/local/lib)
add_executable(zoomsdk main.cpp
                       wsconnectionmanager.h
                       wsconnectionmanager.cpp
                       packetscache.h
                       packetscache.cpp
)

target_link_libraries(zoomsdk PRIVATE curl)
