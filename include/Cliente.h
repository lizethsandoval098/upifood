#ifndef CLIENTE_H
#define CLIENTE_H

#include <string>
#include <vector>
#include <iostream>

#include "Ventana.h"
#include "Tarjeta.h"
#include "Pedido.h"
#include "BaseDatos.h"

using namespace std;

class Cliente{

	private:
	       // conexion	
		int socketCliente; 
		string ipServidor; 
		int  puerto;

		//
		string tipoCliente; // upiita, ipn, invitado
		vector<Tarjeta> tarjetasGuardadas;
		vector<Pedido> historialPedidos;
		Pedido pedidoActual;

	public: 
		Cliente(); 
		bool conectar();
	       
		~Cliente();
	        
		string getTipoCliente();
		vector<Tarjeta> getTarjetasGuardadas();
		Pedido& getPedidoActual();
		
		void setTipoCliente();
		void cargarTarjetasGuardadas();
		void cargarPedidoActual();
	
		void verHistorialP();
		void hacerPedido();
		void pagar();
		void recogerPedido();
};

#endif
