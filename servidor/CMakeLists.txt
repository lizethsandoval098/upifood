cmake_minimum_required(VERSION 3.10)

project(proyectof)

set(CMAKE_CXX_STANDARD 17)

add_executable(
	servidor 
	src/main.cpp
	src/Servidor.cpp
)
