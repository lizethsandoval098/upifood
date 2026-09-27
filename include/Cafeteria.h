#ifndef CAFETERIA_H
#define CAFETERIA_H

#include <string>
#include <vector>
#include <iostream>

#include "Usuario.h"
#include "Producto.h"
#include "Pedido.h"

using namespace std;

// Cafeteria hereda de Usuario: nombre, correo, contrasenaHash, username y
// tipoUsuario ("Cafe") ya vienen de la clase base.
class Cafeteria : public Usuario {
	private:
		string idCafeteria;
		vector<Producto> inventario;
		vector<Pedido> listaPedidos;

		// para la barra de "Atendiendo cajas..." (ventas directas sin pedido/QR)
		float gananciaCajaTurno;
		vector<Producto> vendidosCajaTurno;

	public:
		Cafeteria();
		Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria);

		~Cafeteria();

		string getNombreCafeteria() const;
		string getIdCafeteria() const;
		vector<Producto> getInventario() const;
		vector<Pedido> getListaPedidos() const;
		float getGananciaCajaTurno() const;
		vector<Producto> getVendidosCajaTurno() const;

		void setIdCafeteria(const string& id);

		void cargarInventario();
		void verInventario();
		void cargarListaPedidos();

		void restockProducto(const string& idProducto, int cantidad);

		// Estas dos hacen el trabajo de verdad; gestionarPedido() las orquesta.
		void elaborarPedido(const string& folio);
		void entregarPedido(const string& folio);
		void gestionarPedido(const string& folio);

		void escanearQR(const string& codigo);
		void verificarPago(const string& folio);

		// Venta directa en caja (sin pasar por el flujo de pedido con QR).
		void atenderCajas(const Producto& producto, int cantidad);
};

#endif
