#include "BaseDatos.h"
#include <iostream>

using namespace std;

BaseDatos::BaseDatos(){
	nombreBD = "upiifood.db";
	db = nullptr;
}

BaseDatos::~BaseDatos(){ }
	desconectar();
}

bool BaseDatos::conectar(){
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

void BaseDatos::desconectar(){
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

bool BaseDatos::inicializarTablas(){
	string sqlUsuarios = "CREATE TABLE IF NOT EXISTS Usuarios ("
			     "username TEXT PRIMARY KEY, "
			     "tipoUsuario TEXT NOT NULL CHECK (tipoUsuario IN "
			     "('Admin', 'Cafe', 'Cliente')), "
			     "nombre TEXT NOT NULL, "
			     "correo TEXT NOT NULL CHECK (correo LIKE '%@alumno.ipn.mx'), "
			     "contrasena NOT NULL CHECK (length(contrasena) >= 8));";
	
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
				  "FOREIGN KEY (idProducto) REFERENCES Productos(id) "
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
			     "numeroTarjeta INTEGER PRIMARY KEY, "
			     "usernameCliente TEXT NOT NULL, "
			     "nombrePropietario TEXT NOT NULL, "
			     "fechaVencimiento TEXT NOT NULL CHECK (fechaVencimiento "
			     "LIKE '__/__'), "
			     "CVV INTEGER NOT NULL, "
			     "FOREIGN KEY (usernameCliente, nombrePropietario) "
			     "REFERENCES Usuarios(username, nombre) "
			     "ON DELETE CASCADE ON UPDATE CASCADE);";

	return ejecutarQuery(sqlUsuarios) && ejecutarQuery(sqlCafeterias) && ejecutarQuery(sqlClientes) && ejecutarQuery(sqlProductos) && ejecutarQuery(sqlPedidos) && ejecutarQuery(sqlDetallePedido) && ejecutarQuery(sqlPagos) && ejecutarQuery(sqlTarjetas);
}


// Usuarios
bool BaseDatos::guardarUsuarioCliente(Cliente& cliente){     // guardar en db
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
	
vector<Usuario> obtenerUsuarios();        // para admin
		Usuario obtenerUsuario(string username);  // usuario especifico
		
		// Pedidos
bool BaseDatos::guardarPedido(Pedido& pedido){
	ejecutarQuery("BEGIN TRANSACTION;");

	string sqlP = "INSERT INTO Pedidos (folio, fecha, urlQR, estado, total, usernameCliente, "
		+ "idCafeteria) VALUES ('" + pedido.getFolio() + "',' " + pedido.getFecha() + 
		"', '" + pedido.getUrlQR() + "', '" + pedido.getEstado() + "', " + 
		to_string(pedido.getTotal()) + ", '" + pedido.getUsernameCliente() + 
		"', " + to_string(pedido.getIdCafeteria()) + ");";

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

		vector<Pedido> obtenerPedidos();
		Pedido obtenerPedido(string folio);
		
		// Productos
bool BaseDatos::guardarProducto(Producto& producto){

	string sqlP = "INSERT INTO Productos (idProducto, nombreProducto, cantidad, precio, total) "
		+ "VALUES ('" + producto.getIdProducto() + "',' " + pedido.getFecha() + 
		"', '" + pedido.getUrlQR() + "', '" + pedido.getEstado() + "', " + 
		to_string(pedido.getTotal()) + ", '" + pedido.getUsernameCliente() + 
		"', " + to_string(pedido.getIdCafeteria()) + ");";

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
		vector<Producto> obtenerInventario(int idCafeteria);
		bool actualizarExistencia(int idProducto, int cantidad);
		int obtenerExistencia(int idProducto);
		
		// Pagos
		bool guardarPago(Pago pago);
		Pago obtenerPago(int folio);


