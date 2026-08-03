#ifndef CLIENTE_H
#define CLIENTE_H

#include<string>

class Cliente{

	private: 
		int socketCliente; 
		std::string ipServidor; 
		int  puerto; 
	public: 
		Cliente(); 
		bool conectar(); 
};

#endif
