cmake_minimum_required(VERSION 3.21)

project(AfVideoPlayer
    VERSION 0.1.35
    DESCRIPTION "Desktop video player for watching movies with subtitles, aimed at language learning"
    HOMEPAGE_URL "https://github.com/andrew2020wit/af-video-player-v2"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network OpenGL Qml Quick QuickControls2 Test)
qt_standard_project_setup(REQUIRES 6.8)
qt_policy(SET QTP0004 NEW)

include(cmake/Dependencies.cmake)

if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(src/core)
add_subdirectory(src/export)
add_subdirectory(src/app)

enable_testing()
add_subdirectory(tests)
