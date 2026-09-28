cmake_minimum_required(VERSION 3.16)

project(upiifood)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# SFML es OPCIONAL: si no esta instalada, todo compila igual (sin las ventanas).
# Las pantallas de consola "MONITOR EN VIVO" no necesitan SFML.
find_package(SFML 2.5 COMPONENTS graphics window system QUIET)

set(FUENTES_MODELO
	src/Usuario.cpp src/Cliente.cpp src/Cafeteria.cpp src/Administrador.cpp
	src/Producto.cpp src/Pedido.cpp src/Tarjeta.cpp src/Pago.cpp
	src/database.cpp src/Protocolo.cpp src/Senales.cpp src/Monitor.cpp
)

if(SFML_FOUND)
	list(APPEND FUENTES_MODELO src/Ventana.cpp)
endif()

add_library(modelo STATIC ${FUENTES_MODELO})
target_link_libraries(modelo PUBLIC SQLite3::SQLite3 Threads::Threads)

if(SFML_FOUND)
	target_compile_definitions(modelo PUBLIC UPIIFOOD_CON_SFML)
	target_link_libraries(modelo PUBLIC sfml-graphics sfml-window sfml-system)
	message(STATUS "SFML encontrada: se incluyen las ventanas graficas")
else()
	message(STATUS "SFML no encontrada: se compila solo con consola (todo lo demas funciona)")
endif()

add_executable(cliente src/mainCliente.cpp)
target_link_libraries(cliente PRIVATE modelo)

add_executable(cafeteria src/mainCafeteria.cpp)
target_link_libraries(cafeteria PRIVATE modelo)

add_executable(administrador src/mainAdministrador.cpp)
target_link_libraries(administrador PRIVATE modelo)

# servidor: el UNICO que abre la base de datos directamente
add_executable(servidor src/mainServidor.cpp src/Servidor.cpp)
target_link_libraries(servidor PRIVATE modelo)

# simulador: clientes virtuales (fork + hilos + pipe + senales)
add_executable(simulador src/mainSimulador.cpp src/Simulador.cpp)
target_link_libraries(simulador PRIVATE modelo)
