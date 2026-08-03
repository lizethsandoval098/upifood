#ifndef CLIENTE_H
#define CLIENTE_H

#include<string>

using namespace std;

class Cliente{

	private:
	       // conexion	
		int socketCliente; 
		string ipServidor; 
		int  puerto;

		//
		string tipoCliente; // upiita, ipn, invitado

	public: 
		Cliente(); 
		bool conectar();
	       // destructor
	       // getter
	       // setter
	       // hacerpedido
	       // pagar
	       // recogerpedido 	
};

#endif
