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
	       
		~Cliente();
	        
		string getTipoCliente();
		void setTipoCliente();
		
		void hacerPedido();
		void pagar();
		void recogerPedido();
};

#endif
