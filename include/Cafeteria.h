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
		strin idCafeteria;
		string nombreCafeteria;
		vector<Producto> inventario;
		vector<Pedido> listaPedidos;

	public:
		Cafeteria();

		Cafeteria(string idCafeteria, string nombreCafeteria);

		~Cafeteria();

		string getNombreCafeteria();
		string getIdCafeteria();

		void setNombreCafeteria();
		void setIdCafeteria();
		void cargarInventario();
		void cargarListaPedidos();

		void restockProducto();
		void elaborarPedido();
		void entregarPedido();
		void escanearQR();
		void verificarPago();
};

#endif
		
