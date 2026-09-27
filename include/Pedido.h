#ifndef PEDIDO_H
#define PEDIDO_H

#include <string>
#include <vector>
#include <utility> // pair
#include <iostream>
#include "Producto.h"

using namespace std;

class Pedido{
	private:
		string folio;
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

		void setFolio(const string& f);
		void setFecha(const string& f);
		void setEstado(const string& e);
		void setUsernameCliente(const string& uc);
		void setIdCafeteria(const string& ic);
		void setTotal(float t);
		void setListaProductos(const vector<pair<Producto, int>>& lista);

		void agregarProducto(const Producto& producto, int cantidad);
		bool eliminarProducto(const string& idp);
		void mostrarCarrito();
		void vaciarCarrito();

		void generarTotal();
		void generarTicket();
		void cambiarEstado(const string& nuevo);
		void visualizar();

		// El folio expira 1 hora (simulada) despues de generado.
		bool haExpirado(long segundosSimuladosTranscurridos) const;
};

#endif
