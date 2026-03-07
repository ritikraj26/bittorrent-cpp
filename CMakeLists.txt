cmake_minimum_required(VERSION 3.13)

project(codecrafters-bittorrent)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# OpenSSL
find_package(OpenSSL REQUIRED)

# CURL (ADD THIS)
find_package(CURL REQUIRED)

add_executable(bittorrent ${SOURCE_FILES})

target_link_libraries(bittorrent
    PRIVATE
    OpenSSL::Crypto
    CURL::libcurl
)

target_include_directories(bittorrent PRIVATE ${CMAKE_SOURCE_DIR}/src)