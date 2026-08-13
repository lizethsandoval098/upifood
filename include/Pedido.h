#ifndef PEDIDO_H 
#define PEDIDO_H

#include <string>
#include <vector> 
#include <utility> // pair
#include <iostream>
#include "CodigoQR.h"
#include "Producto.h"

class BaseDatos;

using namespace std;

class Pedido{
	private:
		string folio;
		CodigoQR& qr; // url, esValido
		string fecha;
		string estado;
		string usernameCliente;
		string idCafeteria;
		float total;
		vector<pair<Producto, int>> listaProductos; 

		BaseDatos& bd;

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
		CodigoQR getQr() const;
		vector<pair<Producto, int>> getListaProductos() const;

		void setFolio(const string& f);
		void setFecha(const string& f);
		void setEstado(const string& e);
		void setUsernameCliente(const string& uc);
		void setIdCafeteria(const string& ic);
		void setTotal(float t);
		void setQr(const string& url, bool valido);
		
		void cargarListaProductos();		
		void agregarProducto();
		void generarTotal();
		void generarTicket();
};

#endif
		
