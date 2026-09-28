#include "BaseDatos.h"
#include "Usuario.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Producto.h"
#include "Pedido.h"
#include "Pago.h"
#include "Tarjeta.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

BaseDatos::BaseDatos() {
	nombreBD = "upifood.db";
	db = nullptr;
}

BaseDatos::~BaseDatos() {
	desconectar();
}

bool BaseDatos::conectar() {
	// si el archivo no existe, lo crea en la carpeta del proyecto
	int resultado = sqlite3_open(nombreBD.c_str(), &db);

	if(resultado != SQLITE_OK){
		cerr << "Error SQLite. No se pudo abrir la BD." << endl;
		return false;
	}

	cout << "Conexion a SQLite exitosa (" << nombreBD << ")" << endl;
	if (!ejecutarQuery("PRAGMA foreign_keys = ON;")) {
		return false;
	}

	return inicializarTablas();
}

void BaseDatos::desconectar() {
	if(db != nullptr){
		sqlite3_close(db);
		db = nullptr;
	}
}

bool BaseDatos::ejecutarQuery(const string& query) {
	char* errorMsg = nullptr;
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errorMsg);

	if(rc != SQLITE_OK) {
		cerr << "Error SQL: " << (errorMsg ? errorMsg : "desconocido") << endl;
		sqlite3_free(errorMsg);

		return false;
	}

	return true;
}

bool BaseDatos::inicializarTablas() {
	ejecutarQuery("BEGIN TRANSACTION;");

	string sqlUsuarios = "CREATE TABLE IF NOT EXISTS Usuarios ("
			     "username TEXT PRIMARY KEY, "
			     "tipoUsuario TEXT NOT NULL CHECK (tipoUsuario IN "
			     "('Admin', 'Cafe', 'Cliente')), "
			     "nombre TEXT NOT NULL, "
			     "correo TEXT NOT NULL, "
			     "contrasena TEXT NOT NULL);";

	string sqlCafeterias = "CREATE TABLE IF NOT EXISTS Cafeterias ("
			       "username TEXT PRIMARY KEY, "
			       "idCafeteria TEXT NOT NULL UNIQUE, "
			       "nombreCafeteria TEXT NOT NULL, "
			       "FOREIGN KEY (username) REFERENCES Usuarios(username) "
			       "ON DELETE CASCADE);";

	string sqlClientes = "CREATE TABLE IF NOT EXISTS Clientes ("
			     "username TEXT PRIMARY KEY, "
			     "apellidoPaterno TEXT, "
			     "apellidoMaterno TEXT, "
			     "tipoCliente TEXT NOT NULL CHECK "
			     "(tipoCliente IN ('UPIITA', 'IPN', 'EXTERNO', 'INVITADO')), "
			     "FOREIGN KEY (username) REFERENCES Usuarios(username) "
			     "ON DELETE CASCADE);";

	string sqlProductos = "CREATE TABLE IF NOT EXISTS Productos ("
			      "idProducto TEXT PRIMARY KEY, "
		      	      "nombreProducto TEXT NOT NULL, "
			      "stock INTEGER NOT NULL CHECK (stock >=0), "
			      "precio REAL NOT NULL CHECK (precio > 0.0));";

	string sqlPedidos = "CREATE TABLE IF NOT EXISTS Pedidos ("
			    "folio TEXT PRIMARY KEY, "
		    	    "fecha TEXT DEFAULT (DATETIME('now', 'localtime')), "
			    "estado TEXT NOT NULL DEFAULT 'Pendiente' CHECK (estado IN "
			    "('Pendiente', 'Preparando', 'Listo', 'Entregado', 'Cancelado')), "
			    "total REAL NOT NULL CHECK (total > 0.0), "
			    "usernameCliente TEXT NOT NULL, "
			    "idCafeteria TEXT NOT NULL, "
			    "FOREIGN KEY (usernameCliente) REFERENCES Usuarios(username) "
			    "ON DELETE RESTRICT, "
			    "FOREIGN KEY (idCafeteria) REFERENCES Cafeterias(idCafeteria) ON DELETE "
			    "RESTRICT);";

	string sqlDetallePedido = "CREATE TABLE IF NOT EXISTS DetallePedido ("
				  "folioPedido TEXT NOT NULL, "
				  "idProducto TEXT NOT NULL, "
				  "cantidadP INTEGER NOT NULL CHECK (cantidadP >0), "
				  "precioUnitario REAL NOT NULL CHECK (precioUnitario > 0.0), "
				  "PRIMARY KEY (folioPedido, idProducto), "
				  "FOREIGN KEY (folioPedido) REFERENCES Pedidos(folio) "
				  "ON DELETE RESTRICT, "
				  "FOREIGN KEY (idProducto) REFERENCES Productos(idProducto) "
				  "ON DELETE RESTRICT);";

  	string sqlPagos = "CREATE TABLE IF NOT EXISTS Pagos ("
			 "folioPedido TEXT NOT NULL, "
			 "monto REAL NOT NULL CHECK (monto > 0.0), "
			 "aprobado INTEGER NOT NULL CHECK (aprobado IN (0,1)), "
			 "numTarjeta TEXT, "
			 "FOREIGN KEY (folioPedido) REFERENCES Pedidos(folio) ON DELETE RESTRICT, "
			 "FOREIGN KEY (numTarjeta) REFERENCES Tarjetas(numeroTarjeta) "
			 "ON DELETE SET NULL);";

	string sqlTarjetas = "CREATE TABLE IF NOT EXISTS Tarjetas ("
			     "numeroTarjeta TEXT NOT NULL, "
			     "usernameCliente TEXT NOT NULL, "
			     "fechaVencimiento TEXT NOT NULL, "
			     "CVV TEXT NOT NULL, "
			     "FOREIGN KEY (usernameCliente) "
			     "REFERENCES Usuarios(username) "
			     "ON DELETE CASCADE ON UPDATE CASCADE);";

	bool resultado = ejecutarQuery(sqlUsuarios) &&
			 ejecutarQuery(sqlCafeterias) &&
			 ejecutarQuery(sqlClientes) &&
			 ejecutarQuery(sqlProductos) &&
			 ejecutarQuery(sqlPedidos) &&
			 ejecutarQuery(sqlDetallePedido) &&
			 ejecutarQuery(sqlTarjetas) &&
			 ejecutarQuery(sqlPagos);

	if(resultado) {
		ejecutarQuery("COMMIT;");
		return true;
	} else {
		ejecutarQuery("ROLLBACK;");
		return false;
	}
}


// =====================================================================
// Usuarios / Clientes
// =====================================================================

bool BaseDatos::guardarUsuarioCliente(const Cliente& cliente) {
	ejecutarQuery("BEGIN TRANSACTION;");

	sqlite3_stmt* stmt;
	string sqlU = "INSERT INTO Usuarios (username, tipoUsuario, nombre, correo, contrasena) "
		      "VALUES (?, 'Cliente', ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlU.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de usuario: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, cliente.getUsername().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, cliente.getNombre().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, cliente.getCorreo().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, cliente.getContrasena().c_str(), -1, SQLITE_TRANSIENT);

	bool exitoU = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exitoU) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	string sqlC = "INSERT INTO Clientes (username, apellidoPaterno, apellidoMaterno, tipoCliente) "
		      "VALUES (?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlC.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de cliente: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, cliente.getUsername().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, cliente.getApellidoPaterno().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, cliente.getApellidoMaterno().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, cliente.getTipoCliente().c_str(), -1, SQLITE_TRANSIENT);

	bool exitoC = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exitoC) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	ejecutarQuery("COMMIT;");

	return true;
}

// =====================================================================
// Usuarios / Clientes
// =====================================================================

bool BaseDatos::guardarAdministrador(const Usuario& admin) {
	sqlite3_stmt* stmt;
	string sqlU = "INSERT INTO Usuarios (username, tipoUsuario, nombre, correo, contrasena) "
		      "VALUES (?, 'Admin', ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlU.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de administrador: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, admin.getUsername().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, admin.getNombre().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, admin.getCorreo().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, admin.getContrasena().c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	return exito;
}

Usuario BaseDatos::obtenerAdministrador(const string& username) {
	Usuario admin;

	string sql = "SELECT nombre, correo, contrasena "
		     "FROM Usuarios "
		     "WHERE username = ? AND tipoUsuario = 'Admin';";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el administrador: " << sqlite3_errmsg(db) << endl;
		return admin;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		admin.setTipoUsuario("Admin");
		admin.setNombre(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		admin.setCorreo(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		admin.setContrasena(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		admin.setUsername(username);
	}

	sqlite3_finalize(stmt);

	return admin;
}

vector<Usuario> BaseDatos::obtenerUsuarios() {        // para admin
	vector<Usuario> listaU;

	string sql = "SELECT tipoUsuario, nombre, correo, contrasena, username "
	             "FROM Usuarios ORDER BY rowid ASC;"; // rowid ASC = orden de registro

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la lista de Usuarios: " << sqlite3_errmsg(db) << endl;
		return listaU;
	}

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		string tipoUsuario = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		string nombre = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		string correo = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		string contrasena = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

		Usuario u(tipoUsuario, nombre, correo, "", username);
		u.setContrasena(contrasena); // sobrescribe el hash de "" con el real
		listaU.push_back(u);
	}

	sqlite3_finalize(stmt);

	return listaU;
}

Cliente BaseDatos::obtenerUsuarioCliente(const string& username) {
	Cliente cliente;

	string sql1 = "SELECT U.nombre, U.correo, U.contrasena, C.apellidoPaterno, "
		     "C.apellidoMaterno, C.tipoCliente "
	       	     "FROM Usuarios U "
		     "JOIN Clientes C ON U.username = C.username "
		     "WHERE U.username = ? AND U.tipoUsuario = 'Cliente';";

	sqlite3_stmt* stmt;
	bool encontrado = false;

	if(sqlite3_prepare_v2(db, sql1.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el usuario: " << sqlite3_errmsg(db) << endl;
		return cliente;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		encontrado = true;
		cliente.setTipoUsuario("Cliente");
		cliente.setNombre(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		cliente.setCorreo(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		cliente.setContrasena(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		cliente.setApellidoPaterno(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		cliente.setApellidoMaterno(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		cliente.setTipoCliente(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		cliente.setUsername(username);
	}

	sqlite3_finalize(stmt);

	if(!encontrado) {
		return cliente; // username vacio == "no encontrado" (ver Cliente::iniciarSesionCliente)
	}

	string sql2 = "SELECT numeroTarjeta, fechaVencimiento, CVV "
		      "FROM Tarjetas WHERE usernameCliente = ?;";

	if(sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

		vector<Tarjeta> tarjetas;

		while(sqlite3_step(stmt) == SQLITE_ROW) {
			Tarjeta t;
			t.setNumeroTarjeta(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
			t.setFechaVencimiento(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
			t.setCVV(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
			t.setNombrePropietario(cliente.getNombre());

			tarjetas.push_back(t);
		}

		sqlite3_finalize(stmt);

		cliente.setTarjetasGuardadas(tarjetas);
	}

	cliente.setHistorialPedidos(obtenerHistorialPedidos(username));
	cliente.setPedidoActual(obtenerPedido_Username(username));

	return cliente;
}


// =====================================================================
// Cafeterias
// =====================================================================

bool BaseDatos::guardarCafeteria(const Cafeteria& cafeteria) {
	ejecutarQuery("BEGIN TRANSACTION;");

	sqlite3_stmt* stmt;
	string sqlU = "INSERT INTO Usuarios (username, tipoUsuario, nombre, correo, contrasena) "
		      "VALUES (?, 'Cafe', ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlU.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de usuario (cafeteria): " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, cafeteria.getUsername().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, cafeteria.getNombre().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, cafeteria.getCorreo().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, cafeteria.getContrasena().c_str(), -1, SQLITE_TRANSIENT);

	bool exitoU = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exitoU) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	string sqlC = "INSERT INTO Cafeterias (username, idCafeteria, nombreCafeteria) VALUES (?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlC.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de cafeteria: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, cafeteria.getUsername().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, cafeteria.getIdCafeteria().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, cafeteria.getNombreCafeteria().c_str(), -1, SQLITE_TRANSIENT);

	bool exitoC = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exitoC) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	ejecutarQuery("COMMIT;");

	return true;
}

Cafeteria BaseDatos::obtenerCafeteriaPorUsername(const string& username) {
	Cafeteria cafeteria;

	string sql = "SELECT U.nombre, U.correo, U.contrasena, C.idCafeteria "
		     "FROM Usuarios U "
		     "JOIN Cafeterias C ON U.username = C.username "
		     "WHERE U.username = ? AND U.tipoUsuario = 'Cafe';";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la cafeteria: " << sqlite3_errmsg(db) << endl;
		return cafeteria;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		cafeteria.setTipoUsuario("Cafe");
		cafeteria.setNombre(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		cafeteria.setCorreo(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		cafeteria.setContrasena(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		cafeteria.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		cafeteria.setUsername(username);
	}

	sqlite3_finalize(stmt);

	return cafeteria;
}

vector<Cafeteria> BaseDatos::obtenerCafeterias() {        // para admin
	vector<Cafeteria> listaCaf;

	string sql = "SELECT U.nombre, U.correo, U.username, C.idCafeteria "
	             "FROM Usuarios U "
	             "JOIN Cafeterias C ON U.username = C.username "
	             "ORDER BY U.rowid ASC;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la lista de Cafeterias: " << sqlite3_errmsg(db) << endl;
		return listaCaf;
	}

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		string nombre = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		string correo = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		string idCafeteria = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

		Cafeteria c(nombre, correo, "", username, idCafeteria);
		listaCaf.push_back(c);
	}

	sqlite3_finalize(stmt);

	return listaCaf;
}

int BaseDatos::contarPedidosCafeteria(const string& idCafeteria) {
	string sql = "SELECT COUNT(*) FROM Pedidos WHERE idCafeteria = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al contar pedidos de cafeteria: " << sqlite3_errmsg(db) << endl;
		return 0;
	}

	sqlite3_bind_text(stmt, 1, idCafeteria.c_str(), -1, SQLITE_TRANSIENT);

	int cantidad = 0;
	if(sqlite3_step(stmt) == SQLITE_ROW) {
		cantidad = sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);
	return cantidad;
}


// =====================================================================
// Pedidos
// =====================================================================

bool BaseDatos::guardarPedido(const Pedido& pedido) {
	ejecutarQuery("BEGIN TRANSACTION;");

	sqlite3_stmt* stmt;
	string sqlP = "INSERT INTO Pedidos (folio, fecha, estado, total, usernameCliente, "
		      "idCafeteria) VALUES (?, ?, ?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlP.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de pedido: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, pedido.getFolio().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, pedido.getFecha().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, pedido.getEstado().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 4, pedido.getTotal());
	sqlite3_bind_text(stmt, 5, pedido.getUsernameCliente().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, pedido.getIdCafeteria().c_str(), -1, SQLITE_TRANSIENT);

	bool exitoP = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exitoP) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	for(const auto& item : pedido.getListaProductos()) {
	       	const Producto& producto = item.first;
		int cantidad = item.second;

		string sqlDP = "INSERT INTO DetallePedido (folioPedido, idProducto, cantidadP, "
			       "precioUnitario) VALUES (?, ?, ?, ?);";

		if(sqlite3_prepare_v2(db, sqlDP.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			ejecutarQuery("ROLLBACK;");
			return false;
		}

		sqlite3_bind_text(stmt, 1, pedido.getFolio().c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, producto.getIdProducto().c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 3, cantidad);
		sqlite3_bind_double(stmt, 4, producto.getPrecio());

		bool exitoDP = (sqlite3_step(stmt) == SQLITE_DONE);
		sqlite3_finalize(stmt);

		if(!exitoDP) {
			ejecutarQuery("ROLLBACK;");
			return false;
		}
	}

	ejecutarQuery("COMMIT;");

	return true;
}

vector<pair<Producto, int>> BaseDatos::listaProductosPedido(const string& folio) {
	string sql2 = "SELECT DP.idProducto, DP.cantidadP, DP.precioUnitario, "
		      "P.nombreProducto, P.stock "
		      "FROM DetallePedido DP "
		      "JOIN Productos P ON DP.idProducto = P.idProducto "
		      "WHERE DP.folioPedido = ?;";

	sqlite3_stmt* stmt2;
	vector<pair<Producto, int>> listaProductos;

	if(sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt2, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt2, 1, folio.c_str(), -1, SQLITE_TRANSIENT);

		while(sqlite3_step(stmt2) == SQLITE_ROW) {
			Producto producto;
			producto.setIdProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0)));
			int cantidadP = sqlite3_column_int(stmt2, 1);
			producto.setPrecio(sqlite3_column_double(stmt2, 2));
			producto.setNombreProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 3)));
			producto.setStock(sqlite3_column_int(stmt2, 4));
			listaProductos.push_back({producto, cantidadP});
		}

		sqlite3_finalize(stmt2);
	}

	return listaProductos;
}

vector<Pedido> BaseDatos::obtenerPedidosCafeteria(const string& idCafeteria) {        // para cafeteria
	vector<Pedido> listaP;

	string sql = "SELECT folio, fecha, estado, total, usernameCliente "
	       	     "FROM Pedidos "
		     "WHERE idCafeteria = ? "
		     "ORDER BY rowid DESC;"; // mas recientes primero

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la lista de Pedidos: " << sqlite3_errmsg(db) << endl;
		return listaP;
	}

	sqlite3_bind_text(stmt, 1, idCafeteria.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Pedido p;

		p.setFolio(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		p.setTotal(sqlite3_column_double(stmt, 3));
		p.setUsernameCliente(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		p.setIdCafeteria(idCafeteria);

		p.setListaProductos(listaProductosPedido(p.getFolio()));

		listaP.push_back(p);
	}

	sqlite3_finalize(stmt);

	return listaP;
}

// Cambia el estado de un pedido (Pendiente/Preparando/Listo/Entregado/Cancelado).
// Regresa true solo si de verdad se modifico una fila (el folio existe).
bool BaseDatos::actualizarEstadoPedido(const string& folio, const string& nuevoEstado) {
	string sql = "UPDATE Pedidos SET estado = ? WHERE folio = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al actualizar el estado del pedido: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, nuevoEstado.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, folio.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);

	sqlite3_finalize(stmt);

	return exito;
}

vector<Pedido> BaseDatos::obtenerHistorialPedidos(const string& username) {        // para cliente
	vector<Pedido> historialP;

	string sql = "SELECT folio, fecha, estado, total, idCafeteria "
	       	     "FROM Pedidos "
		     "WHERE usernameCliente = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el historial de Pedidos: " << sqlite3_errmsg(db) << endl;
		return historialP;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Pedido p;

		p.setFolio(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		p.setTotal(sqlite3_column_double(stmt, 3));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		p.setUsernameCliente(username);

		p.setListaProductos(listaProductosPedido(p.getFolio()));

		historialP.push_back(p);
	}

	sqlite3_finalize(stmt);

	return historialP;
}

Pedido BaseDatos::obtenerPedido_Folio(const string& folio) {
	Pedido p;

	string sql = "SELECT usernameCliente, fecha, estado, total, idCafeteria "
	       	     "FROM Pedidos "
		     "WHERE folio = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el pedido: " << sqlite3_errmsg(db) << endl;
		return p;
	}

	sqlite3_bind_text(stmt, 1, folio.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		p.setUsernameCliente(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		p.setTotal(sqlite3_column_double(stmt, 3));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		p.setFolio(folio);

		p.setListaProductos(listaProductosPedido(folio));
	}

	sqlite3_finalize(stmt);

	return p;
}

Pedido BaseDatos::obtenerPedido_Username(const string& username) {
	Pedido p;

	string sql = "SELECT folio, fecha, estado, total, idCafeteria "
	       	     "FROM Pedidos "
		     "WHERE usernameCliente = ? "
		     "ORDER BY rowid DESC "
		     "LIMIT 1;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el pedido: " << sqlite3_errmsg(db) << endl;
		return p;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		p.setFolio(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		p.setTotal(sqlite3_column_double(stmt, 3));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		p.setUsernameCliente(username);

		p.setListaProductos(listaProductosPedido(p.getFolio()));
	}

	sqlite3_finalize(stmt);

	return p;
}


// =====================================================================
// Productos
// =====================================================================

bool BaseDatos::guardarProducto(const Producto& producto) {
	sqlite3_stmt* stmt;
	string sqlP = "INSERT INTO Productos (idProducto, nombreProducto, stock, precio) "
		      "VALUES (?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlP.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de producto: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, producto.getIdProducto().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, producto.getNombreProducto().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 3, producto.getStock());
	sqlite3_bind_double(stmt, 4, producto.getPrecio());

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	return exito;
}

bool BaseDatos::agregarProductoAutomatico(const string& idCafeteria, const string& nombre,
                                         int stock, float precio, string& idAsignado) {
	if ((idCafeteria != "1" && idCafeteria != "2") || nombre.empty() ||
	    nombre.find('|') != string::npos || stock < 0 || !isfinite(precio) || precio <= 0.0f) {
		return false;
	}

	int mayorConsecutivo = 0;
	for (const Producto& producto : obtenerInventario(idCafeteria)) {
		const string& id = producto.getIdProducto();
		if (id.size() == 5 && id[0] == 'C' && id[1] == idCafeteria[0] && id[2] == '-' &&
		    id[3] >= '0' && id[3] <= '9' && id[4] >= '0' && id[4] <= '9') {
			int numero = (id[3] - '0') * 10 + (id[4] - '0');
			mayorConsecutivo = max(mayorConsecutivo, numero);
		}
	}

	const int siguienteConsecutivo = mayorConsecutivo + 1;
	if (siguienteConsecutivo > 20) return false;

	char idBuffer[6];
	snprintf(idBuffer, sizeof(idBuffer), "C%s-%02d", idCafeteria.c_str(), siguienteConsecutivo);
	idAsignado = idBuffer;
	Producto producto(nombre, idAsignado, stock, precio);
	if (!guardarProducto(producto)) {
		idAsignado.clear();
		return false;
	}
	return true;
}

bool BaseDatos::modificarProducto(const string& idProducto, const string& nombre,
                                  int stock, float precio) {
	if (nombre.empty() || nombre.find('|') != string::npos || stock < 0 ||
	    !isfinite(precio) || precio <= 0.0f) return false;

	const string sql = "UPDATE Productos SET nombreProducto = ?, stock = ?, precio = ? "
	                   "WHERE idProducto = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

	sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, stock);
	sqlite3_bind_double(stmt, 3, precio);
	sqlite3_bind_text(stmt, 4, idProducto.c_str(), -1, SQLITE_TRANSIENT);
	const bool actualizado = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) == 1;
	sqlite3_finalize(stmt);
	return actualizado;
}

bool BaseDatos::eliminarProducto(const string& idProducto) {
	const string sql = "DELETE FROM Productos WHERE idProducto = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

	sqlite3_bind_text(stmt, 1, idProducto.c_str(), -1, SQLITE_TRANSIENT);
	const bool eliminado = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) == 1;
	sqlite3_finalize(stmt);
	return eliminado;
}

/*
 *    Los productos se distinguen por cafeteria: C1-01, C1-02, ...
 *    y C2-01, C2-02, ...
 *    idCafeteria en la BD sigue siendo "1" o "2".
 */
vector<Producto> BaseDatos::obtenerInventario(const string& idCafeteria) {        // para cafeteria
	vector<Producto> inventario;
	string patron = "C" + idCafeteria + "-%";

	string sql = "SELECT nombreProducto, stock, precio, idProducto "
	       	     "FROM Productos "
		     "WHERE idProducto LIKE ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el inventario: " << sqlite3_errmsg(db) << endl;
		return inventario;
	}

	sqlite3_bind_text(stmt, 1, patron.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Producto p;

		p.setNombreProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		p.setStock(sqlite3_column_int(stmt, 1));
		p.setPrecio(sqlite3_column_double(stmt, 2));
		p.setIdProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));

		inventario.push_back(p);
	}

	sqlite3_finalize(stmt);

	return inventario;
}

bool BaseDatos::actualizarExistencia(const string& idProducto, int nuevoStock) {
	string sql = "UPDATE Productos "
		     "SET stock = ? "
		     "WHERE idProducto = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al actualizar existencia: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_int(stmt, 1, nuevoStock);
	sqlite3_bind_text(stmt, 2, idProducto.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);

	sqlite3_finalize(stmt);

	return exito;
}


// =====================================================================
// Pagos
// =====================================================================

bool BaseDatos::guardarPago(const Pago& pago) {
	sqlite3_stmt* stmt;
	string sqlP = "INSERT INTO Pagos (folioPedido, monto, aprobado, numTarjeta) "
		      "VALUES (?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlP.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de pago: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, pago.getFolioPedido().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 2, pago.getMonto());
	sqlite3_bind_int(stmt, 3, pago.getAprobado() ? 1 : 0);
	sqlite3_bind_text(stmt, 4, pago.getTarjeta().getNumeroTarjeta().c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	return exito;
}

Pago BaseDatos::obtenerPago(const string& folio) {
	Pago pago;

	string sql1 = "SELECT P.monto, P.aprobado, P.numTarjeta, "
		     "T.CVV, T.fechaVencimiento, U.nombre "
	       	     "FROM Pagos P "
		     "JOIN Tarjetas T ON P.numTarjeta = T.numeroTarjeta "
		     "JOIN Usuarios U ON T.usernameCliente = U.username "
		     "WHERE P.folioPedido = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql1.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el pago: " << sqlite3_errmsg(db) << endl;
		return pago;
	}

	sqlite3_bind_text(stmt, 1, folio.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		pago.setMonto(sqlite3_column_double(stmt, 0));
		pago.setAprobado(sqlite3_column_int(stmt, 1) == 1);
		pago.setFolioPedido(folio);

		Tarjeta t;
		t.setNumeroTarjeta(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		t.setCVV(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		t.setFechaVencimiento(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		t.setNombrePropietario(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));

		pago.asignarTarjeta(t);
	}

	sqlite3_finalize(stmt);

	return pago;
}


// =====================================================================
// Tarjetas
// =====================================================================

bool BaseDatos::guardarTarjeta(const Tarjeta& tarjeta) {
	sqlite3_stmt* stmt;
	string sql = "SELECT username FROM Usuarios WHERE nombre = ?;";

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al buscar el propietario de la tarjeta: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, tarjeta.getNombrePropietario().c_str(), -1, SQLITE_TRANSIENT);

	string username;
	bool encontrado = false;

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		encontrado = true;
	}

	sqlite3_finalize(stmt);

	if(!encontrado) {
		return false;
	}

	string sqlT = "INSERT INTO Tarjetas (numeroTarjeta, usernameCliente, "
		      "fechaVencimiento, CVV) VALUES (?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlT.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de tarjeta: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, tarjeta.getNumeroTarjeta().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, tarjeta.getFechaVencimiento().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, tarjeta.getCVV().c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	return exito;
}
