cmake_minimum_required(VERSION 3.26)
project(kraken_api_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(plugin-core)
add_subdirectory(launcher)
add_subdirectory(ui-native)
add_subdirectory(kraken-ui)
