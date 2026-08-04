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
		int idCafeteria;
		string nombreCafeteria;
		vector<Producto> inventario;
		vector<Pedido> listaPedidos;

	public:
		Cafeteria();

		Cafeteria(int idCafeteria, string nombreCafeteria);

		~Cafeteria();

		string getNombreCafeteria();
		int getIdCafeteria();

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
		
