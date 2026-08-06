#ifndef PEDIDO_H 
#define PEDIDO_H

#include <string>
#include <vector> 
#include <utility> // pair
#include <iostream>
#include "CodigoQR.h"
#include "Producto.h"
#include "BaseDatos.h"

using namespace std;

class Pedido{
	private:
		string folio;
		CodigoQR qr; // url, esValido
		string fecha;
		string estado;
		string usernameCliente;
		string idCafeteria;
		float total;
		vector<pair<Producto, int>> listaProductos; 

	public:
		Pedido();
		Pedido(string folio, string usernameCliente, string idCafeteria, float total,
		      vector<pair<Producto, int>> listaProductos);

		~Pedido();

		string getFolio();
		string getFecha();
		string getEstado();
		string getUsernameCliente();
		string getIdCafeteria();
		float getTotal();
		vector<pair<Producto, int>> getListaProductos();

		void setFolio();
		void setFecha();
		void setEstado();
		void setUsernameCliente();
		void setIdCafeteria();
		void setTotal();
		
		void cargarListaProductos();		
		void agregarProducto();
		void generarTotal();
		void generarTicket();
};

#endif
		
