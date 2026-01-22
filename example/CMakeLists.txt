cmake_minimum_required(VERSION 3.17)

project(MiniLog_SimpleExample)

add_executable(SimpleExample main.cpp)

include(FetchContent)
FetchContent_Declare(
    minilog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../
)
FetchContent_MakeAvailable(minilog)

target_link_libraries(${PROJECT_NAME} PRIVATE minilog::minilog)
