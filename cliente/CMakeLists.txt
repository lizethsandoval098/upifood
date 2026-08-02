cmake_minimum_required(VERSION 3.10)

project(cliente)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)
add_executable(
	cliente 
	src/main.cpp
	src/Cliente.cpp
)
