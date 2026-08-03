#ifndef CAFETERIA_H 
#define CAFETERIA_H

#include <string>
#include <vector>
#include <iostream>
#include "Producto"
#include "Pedido.h"

using namespace std;

class Cafeteria{
	private:
		string nombreCafeteria;
		vector<Producto> inventario;
		vector<Pedido> listaPedidos;

	public:
		Cafeteria();

		Cafeteria(string nombreCafeteria);

		~Cafeteria();

		string getNombreCafeteria();
		
		void setNombreCafeteria();
		void cargarInventario();
		void cargarListaPedidos();

		void restockProducto();
		void elaborarPedido();
		void entregarPedido();
		void escanearQR();
		void verificarPago();
};

#endif
		
