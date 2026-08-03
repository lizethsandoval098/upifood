#ifndef PEDIDO_H 
#define PEDIDO_H

#include <string>
#include <vector> 
#include <utility> // pair
#include <iostream>

using namespace std;

class Pedido{
	public:
		string folio;
		CodigoQR qr;
		string nombreCliente;
		string nombreCafeteria;
		float total;
		vector<pair<Producto, int>> listaProductos; 

	private:
		// constructor
		// destructor
		// getters
		// setters
		// agregarProducto
		// generarTotal
};

#endif
		
