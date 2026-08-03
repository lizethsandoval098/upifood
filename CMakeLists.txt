cmake_minimum_required(VERSION 3.10)

project(cliente)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)

# cliente 
add_executable(
	cliente  
	src/mainCliente.cpp
	src/Cliente.cpp
)

# servidor
add_executable(
	servidor
	src/mainServidor.cpp
	src/Servidor.cpp
)

# administrador
#add_executable(
#	administrador
#	src/mainAdministrador.cpp
#	src/Administrador.cpp
#)
