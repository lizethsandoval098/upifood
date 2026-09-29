#ifndef BASEDATOS_H
#define BASEDATOS_H

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

class Usuario;
class Cliente;
class Cafeteria;
class Pedido;
class Producto;
class Pago;
class Tarjeta;

// Numero de una cafeteria a partir de su ID en la BD: "1" -> 1, "C-01" -> 1,
// "C-02" -> 2. Sirve para armar los IDs de producto (C1-01, C2-01, ...).
// Regresa 0 si el ID no trae un numero entre 1 y 9.
int numeroCafeteria(const string& idCafeteria);

class BaseDatos{
	private:
		string nombreBD;
		sqlite3* db;

		bool ejecutarQuery(const string& query);

	public:
		BaseDatos();

		~BaseDatos();

		bool conectar();
		void desconectar();
		bool inicializarTablas();

		// Usuarios / Clientes
		bool guardarUsuarioCliente(const Cliente& cliente);
		vector<Usuario> obtenerUsuarios();               // para admin
		Cliente obtenerUsuarioCliente(const string& username);
		Usuario obtenerAdministrador(const string& username);
		bool guardarAdministrador(const Usuario& admin);

		// Cafeterias
		vector<Cafeteria> obtenerCafeterias();           // para admin
		int contarPedidosCafeteria(const string& idCafeteria);
		Cafeteria obtenerCafeteriaPorUsername(const string& username);
		bool guardarCafeteria(const Cafeteria& cafeteria);

		// Pedidos
		bool guardarPedido(const Pedido& pedido);
		vector<pair<Producto, int>> listaProductosPedido(const string& folio);
		// Por defecto NO trae los pedidos ya cerrados en un corte de caja; los reportes
		// que necesitan todo el historial pasan incluirCerrados = true.
		vector<Pedido> obtenerPedidosCafeteria(const string& idCafeteria, bool incluirCerrados = false); // para cafeteria
		vector<Pedido> obtenerHistorialPedidos(const string& username);   // para cliente
		Pedido obtenerPedido_Folio(const string& folio);
		Pedido obtenerPedido_Username(const string& username);            // pedido activo mas reciente
		bool actualizarEstadoPedido(const string& folio, const string& nuevoEstado); // para cafeteria
		// Corte de caja: cierra (cerrado = 1) los pedidos Entregados/Cancelados de la
		// cafeteria y guarda el corte. Regresa cuantos se cerraron y cuanto sumaron.
		bool cerrarDiaCafeteria(const string& idCafeteria, float ventasDirectas,
		                        const string& fechaCierre, int& pedidosEntregados,
		                        int& pedidosCancelados, float& totalPedidos);

		// Productos
		bool guardarProducto(const Producto& producto);
		vector<Producto> obtenerInventario(const string& idCafeteria);
		bool actualizarExistencia(const string& idProducto, int nuevoStock);
		bool actualizarProducto(const string& idProducto, const string& nombre,
		                        float precio, int stock);          // modificar nombre/precio/stock
		bool eliminarProducto(const string& idProducto);
		bool productoTienePedidos(const string& idProducto);      // true si aparece en DetallePedido

		// Pagos
		bool guardarPago(const Pago& pago);
		Pago obtenerPago(const string& folio);
		bool pedidoTienePagoAprobado(const string& folio);

		// Tarjetas
		bool guardarTarjeta(const Tarjeta& tarjeta);
		// Tarjeta REAL de un cliente, ligada por username (no por nombre).
		bool guardarTarjetaCliente(const string& username, const Tarjeta& tarjeta);
		// Tarjetas SIMULADAS de los bots (se ligan por username, no por nombre).
		bool guardarTarjetaSimulada(const string& username, const Tarjeta& tarjeta);
		bool obtenerTarjetaSimulada(const string& username, Tarjeta& tarjeta);
		bool obtenerTarjetaPorNumero(const string& numero, Tarjeta& tarjeta, string& usernameDueno);
		bool existeNumeroTarjeta(const string& numero);
};

#endif
