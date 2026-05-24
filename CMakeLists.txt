cmake_minimum_required(VERSION 3.16)
project(EchoRunner C)
set(CMAKE_C_STANDARD 99)

include(FetchContent)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0 GIT_SHALLOW TRUE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(EchoRunner main.c)
target_link_libraries(EchoRunner raylib m)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(EchoRunner GL dl pthread rt X11)
endif()

if(EMSCRIPTEN)
    set_target_properties(EchoRunner PROPERTIES SUFFIX ".html"
        LINK_FLAGS "-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=33554432 --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
endif()
