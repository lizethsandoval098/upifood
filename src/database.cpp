#include "BaseDatos.h"
#include "Usuario.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Producto.h"
#include "Pedido.h"
#include "Pago.h"
#include "Tarjeta.h"

#include <iostream>

using namespace std;

int numeroCafeteria(const string& idCafeteria) {
	size_t pos = idCafeteria.find_first_of("0123456789");

	if(pos == string::npos) {
		return 0;
	}

	int numero = 0;

	while(pos < idCafeteria.size() && idCafeteria[pos] >= '0' && idCafeteria[pos] <= '9') {
		numero = numero * 10 + (idCafeteria[pos] - '0');

		if(numero > 99) {
			return 0;
		}

		pos++;
	}

	return (numero >= 1 && numero <= 9) ? numero : 0;
}

// true si la tabla ya tiene una columna con ese nombre (sirve para migrar bases
// de datos viejas sin borrarlas).
static bool tablaTieneColumna(sqlite3* db, const string& tabla, const string& columna) {
	sqlite3_stmt* info = nullptr;
	string sql = "PRAGMA table_info(" + tabla + ");";
	bool encontrada = false;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &info, nullptr) != SQLITE_OK) {
		return false;
	}

	while(sqlite3_step(info) == SQLITE_ROW) {
		const unsigned char* nombre = sqlite3_column_text(info, 1);

		if(nombre != nullptr && columna == reinterpret_cast<const char*>(nombre)) {
			encontrada = true;
		}
	}

	sqlite3_finalize(info);

	return encontrada;
}

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
			    "cerrado INTEGER NOT NULL DEFAULT 0, "
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
			 "fechaPago TEXT, "
			 "referencia TEXT, "
			 "usernameCliente TEXT, "
			 "idCafeteria TEXT, "
			 "motivo TEXT, "
			 "FOREIGN KEY (folioPedido) REFERENCES Pedidos(folio) ON DELETE RESTRICT, "
			 "FOREIGN KEY (numTarjeta) REFERENCES Tarjetas(numeroTarjeta) "
			 "ON DELETE SET NULL);";

	string sqlTarjetas = "CREATE TABLE IF NOT EXISTS Tarjetas ("
			     "numeroTarjeta TEXT NOT NULL, "
			     "usernameCliente TEXT NOT NULL, "
			     "fechaVencimiento TEXT NOT NULL, "
			     "CVV TEXT NOT NULL, "
			     "titular TEXT, "
			     "esSimulada INTEGER NOT NULL DEFAULT 0, "
			     "fechaCreacion TEXT, "
			     "FOREIGN KEY (usernameCliente) "
			     "REFERENCES Usuarios(username) "
			     "ON DELETE CASCADE ON UPDATE CASCADE);";

	// Historial de cortes de caja (una fila por cada "Cerrar Dia").
	string sqlCierresCaja = "CREATE TABLE IF NOT EXISTS CierresCaja ("
				"idCierre INTEGER PRIMARY KEY AUTOINCREMENT, "
				"idCafeteria TEXT NOT NULL, "
				"fechaCierre TEXT NOT NULL, "
				"pedidosEntregados INTEGER NOT NULL DEFAULT 0, "
				"pedidosCancelados INTEGER NOT NULL DEFAULT 0, "
				"totalPedidos REAL NOT NULL DEFAULT 0, "
				"ventasDirectas REAL NOT NULL DEFAULT 0, "
				"totalCaja REAL NOT NULL DEFAULT 0);";

	bool resultado = ejecutarQuery(sqlUsuarios) &&
			 ejecutarQuery(sqlCafeterias) &&
			 ejecutarQuery(sqlClientes) &&
			 ejecutarQuery(sqlProductos) &&
			 ejecutarQuery(sqlPedidos) &&
			 ejecutarQuery(sqlDetallePedido) &&
			 ejecutarQuery(sqlTarjetas) &&
			 ejecutarQuery(sqlPagos) &&
			 ejecutarQuery(sqlCierresCaja);

	// Migracion: columnas de tarjetas simuladas y de auditoria de pagos
	// (bases creadas antes de esta version). No se pierde ningun dato.
	auto agregarColumna = [&](const string& tabla, const string& columna, const string& definicion) {
		if(resultado && !tablaTieneColumna(db, tabla, columna)) {
			resultado = ejecutarQuery("ALTER TABLE " + tabla + " ADD COLUMN " + columna + " " + definicion + ";");
		}
	};

	agregarColumna("Tarjetas", "titular", "TEXT");
	agregarColumna("Tarjetas", "esSimulada", "INTEGER NOT NULL DEFAULT 0");
	agregarColumna("Tarjetas", "fechaCreacion", "TEXT");
	agregarColumna("Pagos", "fechaPago", "TEXT");
	agregarColumna("Pagos", "referencia", "TEXT");
	agregarColumna("Pagos", "usernameCliente", "TEXT");
	agregarColumna("Pagos", "idCafeteria", "TEXT");
	agregarColumna("Pagos", "motivo", "TEXT");

	// Dos tarjetas simuladas nunca comparten numero (las tarjetas de clientes
	// reales no se restringen para no cambiar como funcionaban antes).
	if(resultado) {
		ejecutarQuery("CREATE UNIQUE INDEX IF NOT EXISTS idx_tarjetas_simuladas_numero "
		              "ON Tarjetas(numeroTarjeta) WHERE esSimulada = 1;");
		ejecutarQuery("CREATE INDEX IF NOT EXISTS idx_pagos_folio ON Pagos(folioPedido);");
	}

	// Migracion: las bases creadas antes del "Cierre de dia" no tienen la
	// columna Pedidos.cerrado. Se agrega sin perder ningun dato.
	if(resultado && !tablaTieneColumna(db, "Pedidos", "cerrado")) {
		resultado = ejecutarQuery("ALTER TABLE Pedidos ADD COLUMN cerrado INTEGER NOT NULL DEFAULT 0;");
	}

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

vector<Pedido> BaseDatos::obtenerPedidosCafeteria(const string& idCafeteria, bool incluirCerrados) {        // para cafeteria
	vector<Pedido> listaP;

	string sql = "SELECT folio, fecha, estado, total, usernameCliente "
	       	     "FROM Pedidos "
		     "WHERE idCafeteria = ? " + string(incluirCerrados ? "" : "AND cerrado = 0 ") + // los cerrados en un corte no se muestran
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

// Corte de caja / cierre de dia de UNA cafeteria.
// En una sola transaccion:
//   1) suma los pedidos Entregados que todavia no estan cerrados,
//   2) marca como cerrados (cerrado = 1) los pedidos Entregados y Cancelados,
//   3) guarda el corte en CierresCaja.
// Los pedidos activos (Pendiente/Preparando/Listo) NO se tocan: siguen en la
// lista y pasan al siguiente turno.
bool BaseDatos::cerrarDiaCafeteria(const string& idCafeteria, float ventasDirectas,
                                   const string& fechaCierre, int& pedidosEntregados,
                                   int& pedidosCancelados, float& totalPedidos) {
	pedidosEntregados = 0;
	pedidosCancelados = 0;
	totalPedidos = 0.0f;

	if(!ejecutarQuery("BEGIN TRANSACTION;")) {
		return false;
	}

	// 1) Consolidar lo que se va a cerrar
	string sqlSuma = "SELECT "
			 "COALESCE(SUM(CASE WHEN estado = 'Entregado' THEN 1 ELSE 0 END), 0), "
			 "COALESCE(SUM(CASE WHEN estado = 'Cancelado' THEN 1 ELSE 0 END), 0), "
			 "COALESCE(SUM(CASE WHEN estado = 'Entregado' THEN total ELSE 0 END), 0) "
			 "FROM Pedidos "
			 "WHERE idCafeteria = ? AND cerrado = 0 "
			 "AND estado IN ('Entregado', 'Cancelado');";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sqlSuma.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al consolidar el corte de caja: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, idCafeteria.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		pedidosEntregados = sqlite3_column_int(stmt, 0);
		pedidosCancelados = sqlite3_column_int(stmt, 1);
		totalPedidos = static_cast<float>(sqlite3_column_double(stmt, 2));
	}

	sqlite3_finalize(stmt);

	// 2) Marcar como cerrados
	string sqlCerrar = "UPDATE Pedidos SET cerrado = 1 "
			   "WHERE idCafeteria = ? AND cerrado = 0 "
			   "AND estado IN ('Entregado', 'Cancelado');";

	if(sqlite3_prepare_v2(db, sqlCerrar.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cerrar los pedidos: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, idCafeteria.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exito) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	// 3) Guardar el corte
	string sqlCorte = "INSERT INTO CierresCaja (idCafeteria, fechaCierre, pedidosEntregados, "
			  "pedidosCancelados, totalPedidos, ventasDirectas, totalCaja) "
			  "VALUES (?, ?, ?, ?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlCorte.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al guardar el corte de caja: " << sqlite3_errmsg(db) << endl;
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	sqlite3_bind_text(stmt, 1, idCafeteria.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, fechaCierre.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 3, pedidosEntregados);
	sqlite3_bind_int(stmt, 4, pedidosCancelados);
	sqlite3_bind_double(stmt, 5, totalPedidos);
	sqlite3_bind_double(stmt, 6, ventasDirectas);
	sqlite3_bind_double(stmt, 7, static_cast<double>(totalPedidos) + ventasDirectas);

	exito = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);

	if(!exito) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	return ejecutarQuery("COMMIT;");
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

/*
 *    Los productos se distinguen por cafeteria: C1-01, C1-02, ...
 *    y C2-01, C2-02, ...
 *    idCafeteria en la BD puede ser "1", "2" o "C-01", "C-02" (el numero manda).
 */
vector<Producto> BaseDatos::obtenerInventario(const string& idCafeteria) {        // para cafeteria
	vector<Producto> inventario;
	// idCafeteria puede ser "1" o "C-01": en los dos casos los productos son C1-01, C1-02, ...
	string patron = "C" + to_string(numeroCafeteria(idCafeteria)) + "-%";

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

// Modifica nombre, precio y existencias de un producto ya guardado.
// Regresa true solo si de verdad se modifico una fila (el producto existe).
bool BaseDatos::actualizarProducto(const string& idProducto, const string& nombre,
                                   float precio, int stock) {
	string sql = "UPDATE Productos "
		     "SET nombreProducto = ?, precio = ?, stock = ? "
		     "WHERE idProducto = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al modificar el producto: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 2, precio);
	sqlite3_bind_int(stmt, 3, stock);
	sqlite3_bind_text(stmt, 4, idProducto.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);

	sqlite3_finalize(stmt);

	return exito;
}

// Borra un producto. (Antes de llamarla, el servidor revisa con
// productoTienePedidos() que no tenga historial de pedidos.)
bool BaseDatos::eliminarProducto(const string& idProducto) {
	string sql = "DELETE FROM Productos WHERE idProducto = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al eliminar el producto: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, idProducto.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);

	sqlite3_finalize(stmt);

	return exito;
}

// true si el producto aparece en el detalle de algun pedido (asi no se
// pierde el historial de pedidos al borrarlo).
bool BaseDatos::productoTienePedidos(const string& idProducto) {
	string sql = "SELECT COUNT(*) FROM DetallePedido WHERE idProducto = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al revisar los pedidos del producto: " << sqlite3_errmsg(db) << endl;
		return true; // ante la duda, NO dejar borrar
	}

	sqlite3_bind_text(stmt, 1, idProducto.c_str(), -1, SQLITE_TRANSIENT);

	int cantidad = 0;

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		cantidad = sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);

	return cantidad > 0;
}


// =====================================================================
// Pagos
// =====================================================================

bool BaseDatos::guardarPago(const Pago& pago) {
	sqlite3_stmt* stmt;
	string sqlP = "INSERT INTO Pagos (folioPedido, monto, aprobado, numTarjeta, fechaPago, "
		      "referencia, usernameCliente, idCafeteria, motivo) "
		      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

	if(sqlite3_prepare_v2(db, sqlP.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de pago: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, pago.getFolioPedido().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 2, pago.getMonto());
	sqlite3_bind_int(stmt, 3, pago.getAprobado() ? 1 : 0);

	// Un rechazo por "tarjeta no registrada" no tiene tarjeta que referenciar.
	if(pago.getTarjeta().getNumeroTarjeta().empty()) {
		sqlite3_bind_null(stmt, 4);
	} else {
		sqlite3_bind_text(stmt, 4, pago.getTarjeta().getNumeroTarjeta().c_str(), -1, SQLITE_TRANSIENT);
	}

	sqlite3_bind_text(stmt, 5, pago.getFechaPago().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, pago.getReferencia().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 7, pago.getUsernameCliente().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 8, pago.getIdCafeteria().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 9, pago.getMotivo().c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);

	if(!exito) {
		cerr << "Error al guardar el pago: " << sqlite3_errmsg(db) << endl;
	}

	sqlite3_finalize(stmt);

	return exito;
}

static string textoColumna(sqlite3_stmt* stmt, int columna) {
	const unsigned char* t = sqlite3_column_text(stmt, columna);
	return t ? reinterpret_cast<const char*>(t) : "";
}

// Regresa el pago MAS RECIENTE del pedido (puede ser un rechazo).
Pago BaseDatos::obtenerPago(const string& folio) {
	Pago pago;

	string sql1 = "SELECT P.monto, P.aprobado, P.numTarjeta, T.CVV, T.fechaVencimiento, "
		      "COALESCE(T.titular, U.nombre), P.fechaPago, P.referencia, "
		      "P.usernameCliente, P.idCafeteria, P.motivo "
		      "FROM Pagos P "
		      "LEFT JOIN Tarjetas T ON P.numTarjeta = T.numeroTarjeta "
		      "LEFT JOIN Usuarios U ON T.usernameCliente = U.username "
		      "WHERE P.folioPedido = ? "
		      "ORDER BY P.rowid DESC LIMIT 1;";

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
		t.setNumeroTarjeta(textoColumna(stmt, 2));
		t.setCVV(textoColumna(stmt, 3));
		t.setFechaVencimiento(textoColumna(stmt, 4));
		t.setNombrePropietario(textoColumna(stmt, 5));

		pago.asignarTarjeta(t);
		pago.setFechaPago(textoColumna(stmt, 6));
		pago.setReferencia(textoColumna(stmt, 7));
		pago.setUsernameCliente(textoColumna(stmt, 8));
		pago.setIdCafeteria(textoColumna(stmt, 9));
		pago.setMotivo(textoColumna(stmt, 10));
	}

	sqlite3_finalize(stmt);

	return pago;
}

bool BaseDatos::pedidoTienePagoAprobado(const string& folio) {
	string sql = "SELECT COUNT(*) FROM Pagos WHERE folioPedido = ? AND aprobado = 1;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text(stmt, 1, folio.c_str(), -1, SQLITE_TRANSIENT);

	bool existe = (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0);
	sqlite3_finalize(stmt);

	return existe;
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


// ---------------------------------------------------------------------
// Tarjetas simuladas (bots)
// ---------------------------------------------------------------------
bool BaseDatos::existeNumeroTarjeta(const string& numero) {
	string sql = "SELECT COUNT(*) FROM Tarjetas WHERE numeroTarjeta = ?;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		return true; // ante la duda, se considera ocupado
	}

	sqlite3_bind_text(stmt, 1, numero.c_str(), -1, SQLITE_TRANSIENT);

	bool existe = (sqlite3_step(stmt) != SQLITE_ROW || sqlite3_column_int(stmt, 0) > 0);
	sqlite3_finalize(stmt);

	return existe;
}

// Guarda la tarjeta simulada de un bot ligada a SU username (los bots
// comparten el mismo nombre, asi que aqui no se busca el dueno por nombre).
bool BaseDatos::guardarTarjetaSimulada(const string& username, const Tarjeta& tarjeta) {
	string sql = "INSERT INTO Tarjetas (numeroTarjeta, usernameCliente, fechaVencimiento, CVV, "
		     "titular, esSimulada, fechaCreacion) "
		     "VALUES (?, ?, ?, ?, ?, 1, DATETIME('now', 'localtime'));";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al preparar insercion de tarjeta simulada: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_text(stmt, 1, tarjeta.getNumeroTarjeta().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, tarjeta.getFechaVencimiento().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, tarjeta.getCVV().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, tarjeta.getNombrePropietario().c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);

	if(!exito) {
		cerr << "Error al guardar la tarjeta simulada: " << sqlite3_errmsg(db) << endl;
	}

	sqlite3_finalize(stmt);

	return exito;
}

bool BaseDatos::obtenerTarjetaSimulada(const string& username, Tarjeta& tarjeta) {
	string sql = "SELECT numeroTarjeta, CVV, COALESCE(titular, ''), fechaVencimiento "
		     "FROM Tarjetas WHERE usernameCliente = ? AND esSimulada = 1 "
		     "ORDER BY rowid ASC LIMIT 1;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	bool encontrada = false;

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		tarjeta = Tarjeta(textoColumna(stmt, 0), textoColumna(stmt, 1),
		                  textoColumna(stmt, 2), textoColumna(stmt, 3));
		encontrada = true;
	}

	sqlite3_finalize(stmt);

	return encontrada;
}

bool BaseDatos::obtenerTarjetaPorNumero(const string& numero, Tarjeta& tarjeta, string& usernameDueno) {
	string sql = "SELECT T.numeroTarjeta, T.CVV, COALESCE(T.titular, U.nombre, ''), "
		     "T.fechaVencimiento, T.usernameCliente "
		     "FROM Tarjetas T LEFT JOIN Usuarios U ON T.usernameCliente = U.username "
		     "WHERE T.numeroTarjeta = ? ORDER BY T.rowid ASC LIMIT 1;";
	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text(stmt, 1, numero.c_str(), -1, SQLITE_TRANSIENT);

	bool encontrada = false;

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		tarjeta = Tarjeta(textoColumna(stmt, 0), textoColumna(stmt, 1),
		                  textoColumna(stmt, 2), textoColumna(stmt, 3));
		usernameDueno = textoColumna(stmt, 4);
		encontrada = true;
	}

	sqlite3_finalize(stmt);

	return encontrada;
}
