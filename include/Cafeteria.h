#ifndef CAFETERIA_H
#define CAFETERIA_H

#include <string>
#include <vector>
#include <iostream>

#include "Usuario.h"
#include "Producto.h"
#include "Pedido.h"

using namespace std;

// Una venta hecha en caja (para mostrar "ultimas ventas" en la ventana).
struct VentaCaja {
	string nombreProducto;
	int cantidad;
	float subtotal;
};

// Cafeteria hereda de Usuario: nombre, correo, contrasena, username y
// tipoUsuario ("Cafe") ya vienen de la clase base.
//
// IMPORTANTE: la cafeteria NUNCA abre la base de datos. Todo lo pide y lo
// cambia a traves del SERVIDOR (por el socket), igual que Administrador.
class Cafeteria : public Usuario {
	private:
		string idCafeteria;
		vector<Producto> inventario;
		vector<Pedido> listaPedidos;

		// para la barra de "Atendiendo cajas..." (ventas directas sin pedido/QR)
		float gananciaCajaTurno;
		vector<Producto> vendidosCajaTurno;
		vector<VentaCaja> historialCaja;

		// detalle (productos) del ultimo pedido consultado
		string folioDetalle;
		vector<pair<Producto, int>> detallePedido;

		// conexion al servidor (mismo patron que Cliente / Administrador)
		int socketCafeteria;
		string ipServidor;
		int puerto;

		string ultimoError; // motivo del ultimo fallo (para mostrarlo en la ventana)

		// Manda un comando de tipo lista ("OK|N" + N lineas) y regresa las N lineas.
		bool pedirLista(const string& comando, vector<string>& lineas);

	public:
		Cafeteria();
		Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria);

		// Se puede copiar (el Administrador guarda vectores de Cafeteria),
		// pero la copia NO hereda el socket: solo la original lo cierra.
		Cafeteria(const Cafeteria& otra);
		Cafeteria& operator=(const Cafeteria& otra);

		~Cafeteria();

		void setIpServidor(const string& ip);
		bool conectar();

		string getNombreCafeteria() const;
		string getIdCafeteria() const;
		vector<Producto> getInventario() const;
		vector<Pedido> getListaPedidos() const;
		float getGananciaCajaTurno() const;
		vector<Producto> getVendidosCajaTurno() const;
		const vector<VentaCaja>& getHistorialCaja() const;
		const vector<pair<Producto, int>>& getDetallePedido() const;
		string getFolioDetalle() const;
		string getUltimoError() const;

		void setIdCafeteria(const string& id);

		// ---- Piden los datos al servidor y los guardan (regresan false si fallan) ----
		bool cargarInventario();
		bool cargarListaPedidos();
		bool cargarDetallePedido(const string& folio);

		// ---- Acciones (todas pasan por el servidor; false = revisar getUltimoError) ----
		bool restockProducto(const string& idProducto, int cantidad);
		bool cambiarEstadoPedido(const string& folio, const string& nuevoEstado);
		bool elaborarPedido(const string& folio);   // Pendiente  -> Preparando
		bool marcarListo(const string& folio);      // Preparando -> Listo
		bool entregarPedido(const string& folio);   // Listo      -> Entregado
		bool cancelarPedido(const string& folio);   // Pendiente/Preparando -> Cancelado

		// Venta directa en caja (sin pasar por el flujo de pedido con QR).
		// Descuenta del inventario en el servidor y suma a la ganancia del turno.
		bool venderEnCaja(const string& idProducto, int cantidad);

		// ---- Versiones de consola (cargan y ademas imprimen) ----
		void verInventario();

		void verificarPago(const string& folio);

		// Login desde la ventana (SFML): recibe los datos ya escritos.
		bool iniciarSesionCafeteria(const string& username, const string& contrasena);

		// Login por consola: pregunta con cin y llama al de arriba.
		bool iniciarSesionCafeteria();
};

#endif
