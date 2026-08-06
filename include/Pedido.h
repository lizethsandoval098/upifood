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

		string getFolio() const;
		string getFecha() const;
		string getEstado() const;
		string getUsernameCliente() const;
		string getIdCafeteria() const;
		float getTotal() const;
		vector<pair<Producto, int>> getListaProductos() const;

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
		
