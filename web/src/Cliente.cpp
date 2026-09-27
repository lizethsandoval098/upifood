
#include "Cliente.h"
#include <iostream>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>




Cliente::Cliente(){

	socketCliente = -1; 
	ipServidor ="100.91.99.27"; 
	puerto =5000; 
}

bool Cliente::conectar()
{
    std::cout << "Conectando al servidor..." << std::endl;

   // 1. Crear socket
    socketCliente = socket(AF_INET, SOCK_STREAM, 0);


    if(socketCliente == -1)
    {
        std::cout << "Error creando socket" << std::endl;
        return false;
    }


    std::cout << "Socket creado correctamente" << std::endl;



    // 2. Crear la dirección del servidor
    sockaddr_in direccionServidor;


    // 3. Indicar que usamos IPv4
    direccionServidor.sin_family = AF_INET;


    // 4. Indicar el puerto del servidor
    direccionServidor.sin_port = htons(puerto);


    // 5. Indicar la IP del servidor
    inet_pton(
        AF_INET,
        ipServidor.c_str(),
        &direccionServidor.sin_addr
    );


    // 6. Conectarse al servidor
    if(connect(
        socketCliente,
        (sockaddr*)&direccionServidor,
        sizeof(direccionServidor)
    ) == -1)
    {
        std::cout << "Error conectando al servidor" << std::endl;
        return false;
    }


    std::cout << "Conectado al servidor correctamente" << std::endl;


return true; 
}








