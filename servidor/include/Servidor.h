#ifndef SERVIDOR_H
#define SERVIDOR_H

class Servidor {
	
	private: 
	int socketServidor; 
	int puerto;
	int socketCliente;	
	
	public: 
		Servidor();

		bool iniciar();

}; 

#endif
