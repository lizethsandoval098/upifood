#include <iostream>
#include<sys/socket.h>
#include "../include/Servidor.h"

#include<netinet/in.h>
#include<arpa/inet.h>
#include<cstring>




Servidor ::Servidor()
{
//El -1 indica hay un error y si es distinto de -1 quiere decir que el socket se creo se pone -1  para tener una salida en caso de que no se creara el  socket 
socketServidor = -1; 
puerto=5000;
socketCliente = -1;
}

bool Servidor::iniciar(){
	

	std::cout<<"Iniciando Servidor"<<std::endl; 
//En la creacion de un socket se debe indicar el tipo  de socket y el dominio 
	socketServidor = socket(AF_INET, SOCK_STREAM, 0);
//AF_INET indica que se usaran direcciones IPv4 
//SOCK_STREAM inidica que se usara el protocolo de comunicacion TCP el cual  es un protocolo de comunicacion que garantiza que todos los datos lleguen correctamente 

	if(socketServidor == -1)
	{
		std::cout<<"Error creando sockets"<<std::endl; 
		return false; 
	}
	std::cout<<"Socket creado correctamente"<<std::endl; 
	
	
	sockaddr_in direccionServidor;

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_port = htons(puerto);
	direccionServidor.sin_addr.s_addr = INADDR_ANY;

	memset(&(direccionServidor.sin_zero), 0, 8);


	if(bind(socketServidor,
        (sockaddr*)&direccionServidor,
        sizeof(direccionServidor)) == -1)
{
    std::cout << "Error en bind" << std::endl;
    return false;
}

std::cout << "Puerto asignado correctamente: "
          << puerto << std::endl;




if (listen(socketServidor, 5) == -1)
{
    std::cout << "Error al escuchar conexiones." << std::endl;
    return false;
}

std::cout << "Servidor escuchando en el puerto "
          << puerto << std::endl;

  
sockaddr_in direccionCliente;

socklen_t longitudCliente = sizeof(direccionCliente);


std::cout << "Esperando cliente..." << std::endl;


socketCliente = accept(
    socketServidor,
    (sockaddr*)&direccionCliente,
    &longitudCliente
);


if(socketCliente == -1)
{
    std::cout << "Error aceptando cliente" << std::endl;
    return false;
}


std::cout << "Cliente conectado correctamente" << std::endl;





	  return true; 

}
