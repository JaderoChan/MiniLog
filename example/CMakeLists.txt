cmake_minimum_required(VERSION 3.17)

project(MiniLog_SimpleExample)

add_executable(SimpleExample main.cpp)
target_include_directories(SimpleExample PRIVATE ../include)
