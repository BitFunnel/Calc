cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

add_executable(calc calc.cpp)

set_property(TARGET calc PROPERTY CXX_STANDARD 11)
set_property(TARGET calc PROPERTY CXX_STANDARD_REQUIRED ON)
