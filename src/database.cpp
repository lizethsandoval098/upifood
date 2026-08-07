#include "BaseDatos.h"
#include <iostream>
#include <string>

using namespace std;

BaseDatos::BaseDatos() {
	nombreBD = "upiifood.db";
	db = nullptr;
}

BaseDatos::~BaseDatos(){ }
	desconectar();
}

bool BaseDatos::conectar() {
	// si el archivo no existe, lo crea en la carpeta del proyecto
	int resultado = sqlite3_open(nombreBD.c_str(), &db);

	if(resultado != SQLITE_OK){
		cerr << "Error SQLite. No se pudo abrir la BD : " << sqlite3_errmsg(db) << endl;
		return false;
	}

	cout << "Conexión a SQLite exitosa (" << nombreBD << ")" << endl;
	inicializarTablas();
	
	return true;
}

void BaseDatos::desconectar() {
	if(db != nullptr){
		sqlite3_close(db);
		db = nullptr;

		cout << "Conexión cerrada correctamente." << endl;
	}
}

bool BaseDatos::ejecutarQuery(const string& query) {
	char* errorMsg = nullptr;
	int rc = sqlite3_exec(db, query,c_str(), nullptr, nullptr, &errorMsg);

	if(rc != SQLITE_OK) {
		cerr << "Error SQL. " << errorMsg << endl;
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
			     "correo TEXT NOT NULL CHECK (correo LIKE '%@alumno.ipn.mx'), "
			     "contrasena TEXT NOT NULL CHECK (length(contrasena) >= 8));";
	
	string sqlCafeterias = "CREATE TABLE IF NOT EXISTS Cafeterias ("
			       "username TEXT PRIMARY KEY, "
			       "idCafeteria TEXT NOT NULL UNIQUE, "
			       "nombreCafeteria TEXT NOT NULL, "
			       "FOREIGN KEY (username) REFERENCES Usuarios(username) "
			       "ON DELETE CASCADE);";

	string sqlClientes = "CREATE TABLE IF NOT EXISTS Clientes ("
			     "username TEXT PRIMARY KEY, "
			     "tipoCliente TEXT PRIMARY NOT NULL CHECK "
			     "(tipoCliente IN ('UPIITA', 'IPN', 'INVITADO')), "
			     "FOREIGN KEY (username) REFERENCES Usuarios(username) "
			     "ON DELETE CASCADE);";

	string sqlProductos = "CREATE TABLE IF NOT EXISTS Productos ("
			      "idProducto TEXT PRIMARY KEY, "
		      	      "nombreProducto TEXT NOT NULL, "
			      "cantidad INTEGER NOT NULL CHECK (cantidad >=0), "
			      "precio REAL NOT NULL CHECK (precio > 0.0));";
	
	string sqlPedidos = "CREATE TABLE IF NOT EXISTS Pedidos ("
			    "folio TEXT PRIMARY KEY, "
		    	    "fecha TEXT DEFAULT (DATETIME('now', 'localtime')), "
		            "urlQR TEXT NOT NULL CHECK (urlQR LIKE 'http://%'), "
			    "estado TEXT NOT NULL DEFAULT 'Pendiente' CHECK (estado IN "
			    "('Pendiente', 'Preparando', 'Listo', 'Entregado', 'Cancelado')), "
			    "total REAL NOT NULL CHECK (total > 0.0), "
			    "usernameCliente TEXT NOT NULL, "
			    "idCafeteria TEXT NOT NULL, "
			    "qrValido INTEGER NOT NULL CHECK (qrValido IN (0,1)), "
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
			 "numTarjeta INTEGER, "
			 "FOREIGN KEY (folioPedido) REFERENCES Pedidos(folio) ON DELETE RESTRICT, "
			 "FOREIGN KEY (numTarjeta) REFERENCES Tarjetas(numeroTarjeta) "
			 "ON DELETE SET NULL);";

	string sqlTarjetas = "CREATE TABLE IF NOT EXISTS Tarjetas ("
			     "numeroTarjeta INTEGER NOT NULL, "
			     "usernameCliente TEXT NOT NULL, "
			     "fechaVencimiento TEXT NOT NULL CHECK (fechaVencimiento "
			     "LIKE '__/__'), "
			     "CVV INTEGER NOT NULL, "
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


// Usuarios
bool BaseDatos::guardarUsuarioCliente(const Cliente& cliente) {     // guardar en db
	ejecutarQuery("BEGIN TRANSACTION;");

	string sqlU = "INSERT INTO Usuarios (username, tipoUsuario, nombre, correo, contrasena) "
		    + "VALUES ('" +
		     cliente.getUsername() + "', 'Cliente', '" +
     		     cliente.getNombre() + "', '" + 
		     cliente.getCorreo() + "', '" +
		     cliente.getContrasena() + "');";
	
	if(!ejecutarQuery(sqlU)) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	string sqlC = "INSERT INTO Clientes (username, tipoCliente) VALUES ('" +
		     cliente.getUsername() + "', '" +
     		     cliente.getTipoCliente() + "');";

	if(!ejecutarQuery(sqlC)) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}
	
	ejecutarQuery("COMMIT;");

	return true;
}
	
vector<Usuario> BaseDatos::obtenerUsuarios() const{        // para admin
	vector<Usuario> listaU;

	string sql = "SELECT username, tipoUsuario, nombre, correo, contrasena "
	       	     "FROM Usuarios;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la lista de Usuarios: " << sqlite3_errmsg(db) << endl;
		return listaU;
	}

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Usuario u;

		u.setUsername(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));	
		u.setTipoUsuario(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		u.setNombre(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		u.setCorreo(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		u.setContrasena(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));

		listaU.push_back(u);
	}
	
	sqlite3_finalize(stmt);

	return listaU;	
}
		
Cliente BaseDatos::obtenerUsuarioCliente(const string& username) const {  // usuario especifico
	Cliente usuario;

	string sql1 = "SELECT U.nombre, U.correo, U.contrasena, C.tipoCliente "
	       	     "FROM Usuarios U "
		     "JOIN Clientes C ON U.username = C.username "
		     "WHERE U.username = ? AND U.tipoUsuario = 'Cliente';";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql1.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el usuario: " << sqlite3_errmsg(db) << endl;
		return usuario;
	}

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		usuario.setTipoUsuario("Cliente");	
		usuario.setNombre(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		usuario.setCorreo(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		usuario.setContrasena(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		usuario.setTipoCliente(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		usuario.setUsername(username);
	}
	
	sqlite3_finalize(stmt);

	string sql2 = "SELECT numeroTarjeta, fechaVencimiento, CVV "
		      "FROM Tarjetas WHERE usernameCliente = ?;";

	if(sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

		vector<Tarjeta> tarjetas;

		while(sqlite3_step(stmt) == SQLITE_ROW) {
			Tarjeta t;
			t.setNumeroTarjeta(sqlite3_column_int(stmt, 0));
			t.setFechaVencimiento(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
			t.setCVV(sqlite3_column_int(stmt, 2));
			t.setNombrePropietario(usuario.getNombre());
			
			tarjetas.push_back(t);
		}

		sqlite3_finalize(stmt);

		usuario.setTarjetasGuardadas(tarjetas);
	}

	usuario.setHistorialPedidos(obtenerHistorialPedidos(username));
		
	usuario.setPedidoActual(obtenerPedido_Username(usuario.getUsername()));	
	 
	return usuario;	
}


// Pedidos
bool BaseDatos::guardarPedido(const Pedido& pedido) {
	ejecutarQuery("BEGIN TRANSACTION;");

	string sqlP = "INSERT INTO Pedidos (folio, fecha, urlQR, estado, total, usernameCliente, "
		+ "idCafeteria) VALUES ('" + pedido.getFolio() + "', '" + pedido.getFecha() + 
		"', '" + pedido.getUrlQR() + "', '" + pedido.getEstado() + "', " + 
		to_string(pedido.getTotal()) + ", '" + pedido.getUsernameCliente() + 
		"', '" + pedido.getIdCafeteria() + "');";

	if(!ejecutarQuery(sqlP)) {
		ejecutarQuery("ROLLBACK;");
		return false;
	}

	for(const auto& item : pedido.getListaProductos()) {
	       	Producto producto = item.first; // primer elemento del pair
		int cantidad = item.second; // segundo elemento del pair

		string sqlDP = "INSERT INTO DetallePedido (folioPedido, idProducto, cantidadP, "
			+ "precioUnitario) VALUES ('" + pedido.getFolio() + "', '" + 
			producto.getIdProducto() + "', " + to_string(cantidad) + ", " + 
			to_string(producto.getPrecio()) + ");";
		
		if(!ejecutarQuery(sqlDP)) {
			ejecutarQuery("ROLLBACK;");
			return false;
		}
	}

	ejecutarQuery("COMMIT;");

	return true;
}

vector<pair<Producto, int>> BaseDatos::listaProductosPedido(const string& folio) const{
	string sql2 = "SELECT DP.idProducto, DP.cantidadP, DP.precioUnitario, "
		      "P.nombreProducto P.cantidad "
		      "FROM DetallePedido DP "
		      "JOIN Productos P ON DP.idProducto = P.idProducto "
		      "WHERE DP.folioPedido = ?;";

	sqlite3_stmt* stmt2;
	vector<pair<Producto, int>> listaProductos;
	int cantidadP;

	if(sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt2, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt2, 1, folio.c_str(), -1, SQLITE_TRANSIENT);

		while(sqlite3_step(stmt2) == SQLITE_ROW) {
			Producto producto;
			producto.setIdProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0)));
			cantidadP = sqlite3_column_int(stmt2, 1);
			producto.setCantidad(sqlite3_column_int(stmt2, 4));
			producto.setPrecio(sqlite3_column_double(stmt2, 2));
			producto.setNombreProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 3)));
			listaProductos.push_back({producto, cantidadP});
		}
			
		sqlite3_finalize(stmt2);
	}

	return listaProductos;
}

vector<Pedido> BaseDatos::obtenerPedidosCafeteria(const string& idC) const{        // para cafeteria
	vector<Pedido> listaP;

	string sql = "SELECT folio, fecha, urlQR, estado, total, usernameCliente, qrValido  "
	       	     "FROM Pedidos "
		     "WHERE idCafeteria = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar la lista de Pedidos: " << sqlite3_errmsg(db) << endl;
		return listaP;
	}

	sqlite3_bind_text(stmt, 1, idC.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Pedido p;
		bool ev;

		p.setFolio(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));	
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));

		if(sqlite3_column_int(stmt, 6) == 1) {
			ev = true;
		} else {
			ev = false;
		}

		p.setQr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)), ev);
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		p.setTotal(sqlite3_column_double(stmt, 4));
		p.setUsernameCliente(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		p.setIdCafeteria(idC);
		
		p.setListaProductos(listaProductosPedido(p.getFolio()));

		listaP.push_back(p);
	}
	
	sqlite3_finalize(stmt);

	return listaP;	
}

vector<Pedido> BaseDatos::obtenerHistorialPedidos(const string& uC) const{        // para cliente
	vector<Pedido> historialP;

	string sql = "SELECT folio, fecha, urlQR, estado, total, idCafeteria, qrValido  "
	       	     "FROM Pedidos "
		     "WHERE usernameCliente = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el historial de Pedidos: " << sqlite3_errmsg(db) << endl;
		return historialP;
	}

	sqlite3_bind_text(stmt, 1, uC.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Pedido p;
		bool ev;

		p.setFolio(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));	
		p.setFecha(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));

		if(sqlite3_column_int(stmt, 6) == 1) {
			ev = true;
		} else {
			ev = false;
		}

		p.setQr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)), ev);
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		p.setTotal(sqlite3_column_double(stmt, 4));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		p.setUsernameCliente(uC);
		
		p.setListaProductos(listaProductosPedido(p.getFolio()));

		historialP.push_back(p);
	}
	
	sqlite3_finalize(stmt);

	return historialP;	
}

Pedido BaseDatos::obtenerPedido_Folio(const string& folio) const {
	Pedido p;
	bool ev;

	string sql = "SELECT usernameCliente, fecha, urlQR, estado, total, idCafeteria, qrValido "
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

		if(sqlite3_column_int(stmt, 6) == 1) {
			ev = true;
		} else {
			ev = false;
		}
		
		p.setQr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)), ev);
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		p.setTotal(sqlite3_column_double(stmt, 4));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		p.setFolio(folio);

		p.setListaProductos(listaProductosPedido(p.getFolio()));
	}
	
	sqlite3_finalize(stmt);

	return p;	
}


Pedido BaseDatos::obtenerPedido_Username(const string& username) const {
	Pedido p;
	bool ev;

	string sql = "SELECT folio, fecha, urlQR, estado, total, idCafeteria, qrValido "
	       	     "FROM Pedidos "
		     "WHERE usernameCliente = ? "
		     "ORDER BY rowid DESC "  // columna oculta autoincrementable
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

		if(sqlite3_column_int(stmt, 6) == 1) {
			ev = true;
		} else {
			ev = false;
		}
		
		p.setQr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)), ev);
		p.setEstado(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		p.setTotal(sqlite3_column_double(stmt, 4));
		p.setIdCafeteria(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
		p.setUsernameCliente(username);

		p.setListaProductos(listaProductosPedido(p.getFolio()));
	}
	
	sqlite3_finalize(stmt);

	return p;	
}


// Productos
bool BaseDatos::guardarProducto(const Producto& producto) {
	string sqlP = "INSERT INTO Productos (idProducto, nombreProducto, cantidad, precio) "
		+ "VALUES ('" + producto.getIdProducto() + "', '" + producto.getNombreProducto() + 
		"', " + to_string(producto.getCantidad()) + ", " + to_string(producto.getPrecio()) + 
		");";

	return ejecutarQuery(sqlP);
}


/*
 *    idProducto = idCaf-num,                           galletas de caf 1 : 101-123
 *    idCafeteria = C-101 -> cafeteria 1
 *                  C-202 -> caf 2
 * 
 */


vector<Producto> BaseDatos::obtenerInventario(const string& idCafeteria) const{        // para cafeteria
	vector<Producto> inventario;
	string idProd;
	size_t posGuion = idCafeteria.find('-');

	if(posGuion != string::npos) {  // extrae string despues del guion
		idProd = idCafeteria.substr(posGuion + 1);
	}	

	idProd = idProd + "-%";

	string sql = "SELECT nombreProducto, cantidad, precio, idProducto "
	       	     "FROM Productos "
		     "WHERE idProducto LIKE ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el inventario: " << sqlite3_errmsg(db) << endl;
		return inventario;
	}

	sqlite3_bind_text(stmt, 1, idProd.c_str(), -1, SQLITE_TRANSIENT);

	while(sqlite3_step(stmt) == SQLITE_ROW) {
		Producto p;

		p.setNombreProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));	
		p.setCantidad(sqlite3_column_int(stmt, 1));
		p.setPrecio(sqlite3_column_double(stmt, 2));
		p.setIdProducto(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		
		inventario.push_back(p);
	}
	
	sqlite3_finalize(stmt);

	return inventario;	
}

bool BaseDatos::actualizarExistencia(const string& idProducto, int cantidad) {
	string sql = "UPDATE Productos "
		     "SET cantidad = ? "
		     "WHERE idProducto = ?;";
	sqlite3_stmt* stmt;
	
	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el inventario: " << sqlite3_errmsg(db) << endl;
		return false;
	}

	sqlite3_bind_int(stmt, 1, cantidad);
	sqlite3_bind_text(stmt, 2, idProducto.c_str(), -1, SQLITE_TRANSIENT);

	bool exito = (sqlite3_step(stmt) == SQLITE_DONE);

	sqlite3_finalize(stmt);

	return exito;
}
// int obtenerExistencia(int idProducto);
		
// Pagos
bool BaseDatos::guardarPago(const Pago& pago) {
	string sqlP = "INSERT INTO Pagos (folioPedido, monto, aprobado, numTarjeta) "
		+ "VALUES ('" + pago.getFolioPedido() + "', " + to_string(pago.getMonto()) + 
		", " + to_string(pago.getAprobado()) + ", " + 
		to_string(pago.getTarjeta().getNumeroTarjeta()) + 
		");";

	return ejecutarQuery(sqlP);
} 

Pago BaseDatos::obtenerPago(const string& folio) const{
	Pago pago;
	bool aprob;

	string sql1 = "SELECT P.monto, P.aprobado, P.numTarjeta, "
		     "T.CVV, T.fechaVencimiento, T.usernameCliente, U.nombre"
	       	     "FROM Pagos P "
		     "JOIN Tarjetas T ON P.numTarjeta = T.numeroTarjeta "
		     "JOIN Usuarios U ON T.usernameCliente = U.username "
		     "WHERE P.folioPedido = ? AND U.username = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql1.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar el pago: " << sqlite3_errmsg(db) << endl;
		return pago;
	}
	
	string usern = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

	sqlite3_bind_text(stmt, 1, folio.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, usern.c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		pago.setMonto(sqlite3_column_double(stmt, 0));

		if(sqlite3_column_int(stmt, 1) == 1) {
			aprob = true;
		} else {
			aprob = false;
		}

		pago.setAprobado(aprob);
		pago.setFolio(folio);

		Tarjeta t;
		t.setNumeroTarjeta(sqlite3_column_int(stmt, 2));
		t.setCVV(sqlite3_column_int(stmt, 3));
		t.setNombrePropietario(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
		t.setFechaVencimiento(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));

		pago.setTarjeta(t);
	}
	
	sqlite3_finalize(stmt);

	return pago;
}

// Tarjetas
bool BaseDatos::guardarTarjeta(const Tarjeta& tarjeta) {
	string sql = "SELECT username FROM Usuarios WHERE "
		     "nombre = ?;";

	sqlite3_stmt* stmt;

	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		cerr << "Error al cargar los datos necesarios: " << sqlite3_errmsg(db) << endl;
		return false;
	}
	
	sqlite3_bind_text(stmt, 1, tarjeta.getNombrePropietario().c_str(), -1, SQLITE_TRANSIENT);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
	} else {
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_finalize(stmt);

	string sqlT = "INSERT INTO Tarjetas (numeroTarjeta, usernameCliente, "
		    + "fechaVencimiento, CVV) VALUES (" + 
		    to_string(tarjeta.getNumeroTarjeta()) + 
		    ", '" + username + "', '" + 
		    tarjeta.getFechaVencimiento() + "', " + 
		    to_string(tarjeta.getCVV()) + ");";

	return ejecutarQuery(sqlT);
}
