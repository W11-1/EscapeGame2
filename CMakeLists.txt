cmake_minimum_required(VERSION 3.20)

project(EscapeGame2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

add_executable(EscapeGame2 EscapeGame2.cpp)

target_link_libraries(EscapeGame2 PRIVATE raylib)