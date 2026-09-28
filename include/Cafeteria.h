#ifndef CAFETERIA_H
#define CAFETERIA_H

#include <string>
#include <vector>
#include <iostream>
#include <set>

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
		set<string> foliosPagados;   // folios de listaPedidos cuyo pago fue aprobado

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

		// Todo va por el socket (LISTAR_PEDIDOS, CAMBIAR_ESTADO, ESTADO_PEDIDO, VENTA_CAJA).
		void cargarListaPedidos();
		void verPedidos();
		bool estaPagado(const string& folio) const;

		void restockProducto(const string& idProducto, int cantidad);

		// Ciclo de vida de un pedido:  Pendiente -> Preparando -> Listo -> Entregado
		void elaborarPedido(const string& folio);   // Pendiente  -> Preparando (solo si esta pagado)
		void marcarListo(const string& folio);      // Preparando -> Listo
		void entregarPedido(const string& folio);   // Listo      -> Entregado
		void cancelarPedido(const string& folio);   // (Pendiente | Preparando) -> Cancelado
		void gestionarPedido(const string& folio);  // avanza UN paso segun el estado actual

		void verificarPago(const string& folio);

		// Venta directa en caja (sin pasar por el flujo de pedido con QR).
		// Descuenta stock en el servidor (VENTA_CAJA).
		void atenderCajas(const Producto& producto, int cantidad);

		// Pide username/contrasena por consola y los manda al SERVIDOR por el
		// socket (this->socketCafeteria, ya conectado con conectar()).
		// Llena los datos de *this con la respuesta. Requiere haber llamado
		// conectar() antes.
		bool iniciarSesionCafeteria();
};

#endif
