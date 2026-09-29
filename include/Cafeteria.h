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

// Resumen de un turno / corte de caja (lo que se muestra en pantalla al
// "Cerrar Dia").
struct ResumenCierre {
	int pedidosEntregados = 0;   // pedidos completados que se cierran
	int pedidosCancelados = 0;   // pedidos cancelados que tambien se archivan
	float totalPedidos = 0.0f;   // suma de los pedidos entregados
	float ventasDirectas = 0.0f; // ventas hechas en el mostrador (caja)
	float totalCaja = 0.0f;      // totalPedidos + ventasDirectas
	string fecha;                // fecha y hora del cierre (vacia en la vista previa)
};

// Un producto NUEVO que todavia no se sube a la base de datos.
// No tiene ID: el servidor lo genera al subirlo (C1-01, C1-02, ...).
struct ProductoNuevo {
	string nombre;
	float precio;
	int stock;
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

		// folios de los pedidos de listaPedidos que ya tienen pago aprobado
		vector<string> foliosPagados;

		// productos creados en la ventana que aun NO estan en la base de datos
		vector<ProductoNuevo> productosPendientes;

		// para la barra de "Atendiendo cajas..." (ventas directas sin pedido/QR)
		float gananciaCajaTurno;
		float gananciaPedidosTurno; // la parte de gananciaCajaTurno que viene de pedidos entregados
		vector<Producto> vendidosCajaTurno;
		vector<VentaCaja> historialCaja;

		// detalle (productos) del ultimo pedido consultado
		string folioDetalle;
		vector<pair<Producto, int>> detallePedido;

		// folios de pedidos (web o simulador) ya sumados a gananciaCajaTurno,
		// para no volver a contarlos en cada refresco.
		vector<string> foliosPedidoContabilizados;

		// conexion al servidor (mismo patron que Cliente / Administrador)
		int socketCafeteria;
		string ipServidor;
		int puerto;

		string ultimoError; // motivo del ultimo fallo (para mostrarlo en la ventana)

		// Manda un comando de tipo lista ("OK|N" + N lineas) y regresa las N lineas.
		bool pedirLista(const string& comando, vector<string>& lineas);

		// Revisa listaPedidos por pedidos ya Entregados (hechos desde la web o
		// por el simulador) que todavia no se sumaron a la ganancia del turno.
		void contabilizarPedidosCompletados();

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
		bool hayProductoAgotado() const; // true si algun producto tiene stock 0 (para el aviso en la barra lateral)
		vector<Pedido> getListaPedidos() const;
		bool pedidoPagado(const string& folio) const; // true si el pedido ya tiene pago aprobado
		float getGananciaCajaTurno() const;

		// Vista previa del turno actual (lo que se cerraria si se pulsa "Cerrar Dia").
		ResumenCierre getResumenTurno() const;
		int getPedidosActivos() const;   // pedidos aun sin terminar (pasan al siguiente turno)
		vector<Producto> getVendidosCajaTurno() const;
		const vector<VentaCaja>& getHistorialCaja() const;
		const vector<pair<Producto, int>>& getDetallePedido() const;
		string getFolioDetalle() const;
		string getUltimoError() const;
		const vector<ProductoNuevo>& getProductosPendientes() const;

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
		bool completarPedido(const string& folio);  // avanza paso a paso hasta Entregado

		// ---- Editar el inventario (se guarda en la base de datos via servidor) ----
		// Productos nuevos: primero se juntan en una lista local...
		bool agregarProductoPendiente(const string& nombre, float precio, int stock);
		bool modificarProductoPendiente(size_t indice, const string& nombre, float precio, int stock);
		bool quitarProductoPendiente(size_t indice);
		// ...y este boton los sube TODOS a la base de datos. Regresa cuantos se subieron;
		// los que fallen se quedan en la lista (motivo en getUltimoError()).
		int subirProductosPendientes();

		// Modificar / eliminar un producto que YA esta en la base de datos.
		bool modificarProducto(const string& idProducto, const string& nombre, float precio, int stock);
		bool eliminarProducto(const string& idProducto);

		// Venta directa en caja (sin pasar por el flujo de pedido con QR).
		// Descuenta del inventario en el servidor y suma a la ganancia del turno.
		bool venderEnCaja(const string& idProducto, int cantidad);

		// Cierre de dia / corte de caja: le pide al servidor cerrar los pedidos
		// completados del turno y, si sale bien, reinicia la ganancia del turno a 0.
		// En "resumen" queda el total final (antes del reinicio). false = ver getUltimoError().
		bool cerrarDia(ResumenCierre& resumen);

		// ---- Versiones de consola (cargan y ademas imprimen) ----
		void verInventario();

		void verificarPago(const string& folio);

		// Login desde la ventana (SFML): recibe los datos ya escritos.
		bool iniciarSesionCafeteria(const string& username, const string& contrasena);

		// Login por consola: pregunta con cin y llama al de arriba.
		bool iniciarSesionCafeteria();
};

#endif
