#include "Cafeteria.h"
#include "BaseDatos.h"

using namespace std;

Cafeteria::Cafeteria() {	
}

Cafeteria::Cafeteria(string idCafeteria, string nombreCafeteria)
	: idCafeteria{idCafeteria}, nombreCafeteria{nombreCafeteria} {
	
	// Guarda ID y nombre
	
}

Cafeteria::~Cafeteria() {
	
	// Destructor
	
}

string Cafeteria::getNombreCafeteria() {
	
	return nombreCafeteria;
}

string Cafeteria::getIdCafeteria() {
		
	return idCafeteria;
}

vector<Producto> Cafeteria::getInventario() {
	
	// Devuelve el inventario (productos de cada cafeteria)
	
	return inventario;
}

vector<Pedido> Cafeteria::getListaPedidos() {
	
	// Devuelve los pedidos (de acuerdo a que cafeteria sea la que los usuarios escojan )
	
	return listaPedidos;
}

void Cafeteria::setNombreCafeteria(string nombre) {
		
	nombreCafeteria = nombre;
}

void Cafeteria::setIdCafeteria(string id) {
		
	if(id == "1" || id == "2") {
		
		idCafeteria = id;
	}
	else {
		
		cout << "ID de cafeteria no valido." << endl;
	}
}

void Cafeteria::cargarInventario() {
	
	// Carga el inventario
	
	BaseDatos db;
	
	if(db.conectar()) {
		
		if(idCafeteria == "1") {
			
			inventario = db.obtenerInventario(1);
			
		}
		else if(idCafeteria == "2") {
			
			inventario = db.obtenerInventario(2);
			
		}
		else {
			
			cout << "Cafeteria no valida." << endl;
		}
		
		db.desconectar();
	}
}

void Cafeteria::cargarListaPedidos() {
	
	// Carga los pedidos
	
	BaseDatos db;
	
	if(db.conectar()) {
		
		listaPedidos = db.obtenerPedidos();
		
		db.desconectar();
	}
}

void Cafeteria::restockProducto(int idProducto, int cantidad) {
	
	
	for(int i = 0; i < inventario.size(); i++) {
		
		if(inventario[i].getIdProducto() == idProducto) {
			
			inventario[i].setExistencia(
				inventario[i].getExistencia() + cantidad
			);
			
			cout << "Producto reabastecido correctamente." << endl;
			
			return;
		}
	}
	
	cout << "Producto no encontrado." << endl;
}

void Cafeteria::elaborarPedido(string folio) {
	
	// Busca el pedido
	
	for(int i = 0; i < listaPedidos.size(); i++) {
		
		if(listaPedidos[i].getFolio() == folio) {
			
			cout << "Elaborando pedido: " << folio << endl;
			
			return;
		}
	}
	
	cout << "Pedido no encontrado." << endl;
}

void Cafeteria::entregarPedido(string folio) {
	
	// Busca el pedido
	
	for(int i = 0; i < listaPedidos.size(); i++) {
		
		if(listaPedidos[i].getFolio() == folio) {
			
			cout << "Pedido " << folio << " entregado." << endl;
			
			return;
		}
	}
	
	cout << "Pedido no encontrado." << endl;
}

void Cafeteria::escanearQR(string codigo) {
	
	// Recibe el código QR
	
	cout << "Codigo QR recibido: " << codigo << endl;
}

void Cafeteria::verificarPago(int folio) {
	
	// Consulta el pago
	
	BaseDatos db;
	
	if(db.conectar()) {
		
		Pago pago = db.obtenerPago(folio);
		
		cout << "Pago consultado para el pedido "
		     << folio << endl;
		
		db.desconectar();
	}
}
