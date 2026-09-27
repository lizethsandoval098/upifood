cmake_minimum_required(VERSION 3.16)

project(upiifood)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(Threads REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# ---------------------------------------------------------------------
# Libreria con TODAS las clases del dominio (Usuario, Cliente, Cafeteria,
# Administrador, Producto, Pedido, Tarjeta, Pago, BaseDatos,
# Ventana). La usan cliente/cafeteria/administrador para no repetir la
# lista de .cpp en cada executable.
# ---------------------------------------------------------------------
add_library(
	modelo STATIC
	src/Usuario.cpp
	src/Cliente.cpp
	src/Cafeteria.cpp
	src/Administrador.cpp
	src/Producto.cpp
	src/Pedido.cpp
	src/Tarjeta.cpp
	src/Pago.cpp
	src/database.cpp
	src/Ventana.cpp
	src/Protocolo.cpp
)

target_link_libraries(
	modelo
	PUBLIC
	sqlite3
	sfml-graphics
	sfml-window
	sfml-system
	Threads::Threads
)

# cliente
add_executable(
	cliente
	src/mainCliente.cpp
)
target_link_libraries(cliente PRIVATE modelo)

# cafeteria
add_executable(
	cafeteria
	src/mainCafeteria.cpp
)
target_link_libraries(cafeteria PRIVATE modelo)

# administrador
add_executable(
	administrador
	src/mainAdministrador.cpp
)
target_link_libraries(administrador PRIVATE modelo)

# servidor: el UNICO que abre la base de datos directamente. Necesita las
# mismas clases del dominio para poder atender los comandos (LOGIN_*, etc.)
add_executable(
	servidor
	src/mainServidor.cpp
	src/Servidor.cpp
)
target_link_libraries(servidor PRIVATE modelo)
