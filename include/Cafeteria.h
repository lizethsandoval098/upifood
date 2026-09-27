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

		// conexion al servidor (mismo patron que Cliente)
		int socketCafeteria;
		string ipServidor;
		int puerto;

	public:
		Cafeteria();
		Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria);

		~Cafeteria();

		bool conectar();

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

		void verificarPago(const string& folio);

		// Venta directa en caja (sin pasar por el flujo de pedido con QR).
		void atenderCajas(const Producto& producto, int cantidad);

		// Pide username/contrasena por consola y los manda al SERVIDOR por el
		// socket (this->socketCafeteria, ya conectado con conectar()).
		// Llena los datos de *this con la respuesta. Requiere haber llamado
		// conectar() antes.
		bool iniciarSesionCafeteria();
};

#endif
