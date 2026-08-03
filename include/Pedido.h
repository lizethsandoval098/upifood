#ifndef PEDIDO_H 
#define PEDIDO_H

#include <string>
#include <vector> 
#include <utility> // pair
#include <iostream>
#include "CodigoQR.h"
#include "Producto.h"

using namespace std;

class Pedido{
	private:
		string folio;
		CodigoQR qr;
		string nombreCliente;
		string nombreCafeteria;
		float total;
		vector<pair<Producto, int>> listaProductos; 

	public:
		Pedido();
		Pedido(string folio, string nombreCliente, string nombreCafeteria, float total,
		      vector<pair<Producto, int>> listaProductos);

		~Pedido();

		string getFolio();
		string getNombreCliente();
		string getNombreCafeteria();
		float getTotal();
		vector<pair<Producto, int>> getListaProductos();

		void setFolio();
		void setNombreCliente();
		void setNombreCafeteria();
		void setTotal();
		
		void cargarListaProductos();		
		void agregarProducto();
		void generarTotal();
};

#endif
		
