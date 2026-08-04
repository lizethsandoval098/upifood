#ifndef BASEDATOS_H
#define BASEDATOS_H

#include <string>
#include <vector>
#include <sqlite3.h>

#include "Usuario.h"
#include "Administrador.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Producto.h"
#include "Pedido.h"
#include "Pago.h"
#include "CodigoQR.h"

using namespace std;

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

		// Usuarios
		bool guardarUsuario(Usuario usuario);     // guardar en db
		vector<Usuario> obtenerUsuarios();        // para admin
		Usuario obtenerUsuario(string username);  // usuario especifico
		
		// Pedidos
		bool guardarPedido(Pedido pedido);
		vector<Pedido> obtenerPedidos();
		Pedido obtenerPedido(string folio);
		
		// Productos
		bool guardarProducto(Producto producto);
		vector<Producto> obtenerInventario(int idCafeteria);
		bool actualizarExistencia(int idProducto, int cantidad);
		int obtenerExistencia(int idProducto);
		
		// Pagos
		bool guardarPago(Pago pago);
		Pago obtenerPago(int folio);

};

#endif


