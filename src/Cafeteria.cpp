#include "Cafeteria.h"
#include "BaseDatos.h"
#include "Pago.h"

using namespace std;

Cafeteria::Cafeteria() : gananciaCajaTurno(0.0f) {
}

Cafeteria::Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria)
	: Usuario("Cafe", nombre, correo, contrasena, username),
	  idCafeteria{idCafeteria}, gananciaCajaTurno(0.0f) {
}

Cafeteria::~Cafeteria() {
}

string Cafeteria::getNombreCafeteria() const {
	return getNombre();
}

string Cafeteria::getIdCafeteria() const {
	return idCafeteria;
}

vector<Producto> Cafeteria::getInventario() const {
	return inventario;
}

vector<Pedido> Cafeteria::getListaPedidos() const {
	return listaPedidos;
}

float Cafeteria::getGananciaCajaTurno() const {
	return gananciaCajaTurno;
}

vector<Producto> Cafeteria::getVendidosCajaTurno() const {
	return vendidosCajaTurno;
}

void Cafeteria::setIdCafeteria(const string& id) {
	if(id == "1" || id == "2") {
		idCafeteria = id;
	}
	else {
		cout << "ID de cafeteria no valido." << endl;
	}
}

void Cafeteria::cargarInventario() {
	BaseDatos db;

	if(db.conectar()) {
		inventario = db.obtenerInventario(idCafeteria);
		db.desconectar();
	}
}

void Cafeteria::verInventario() {
	if(inventario.empty()) {
		cout << "Inventario vacio." << endl;
		return;
	}

	for(const auto& p : inventario) {
		cout << "[" << p.getIdProducto() << "] " << p.getNombreProducto()
		     << " - $" << p.getPrecio() << " (stock: " << p.getStock() << ")" << endl;
	}
}

void Cafeteria::cargarListaPedidos() {
	BaseDatos db;

	if(db.conectar()) {
		listaPedidos = db.obtenerPedidosCafeteria(idCafeteria);
		db.desconectar();
	}
}

void Cafeteria::restockProducto(const string& idProducto, int cantidad) {
	for(auto& producto : inventario) {
		if(producto.getIdProducto() == idProducto) {
			int nuevoStock = producto.getStock() + cantidad;
			producto.setStock(nuevoStock);

			BaseDatos db;
			if(db.conectar()) {
				db.actualizarExistencia(idProducto, nuevoStock);
				db.desconectar();
			}

			cout << "Producto reabastecido correctamente." << endl;
			return;
		}
	}

	cout << "Producto no encontrado." << endl;
}

void Cafeteria::elaborarPedido(const string& folio) {
	for(auto& p : listaPedidos) {
		if(p.getFolio() == folio) {
			p.cambiarEstado("Preparando");
			cout << "Elaborando pedido: " << folio << endl;
			return;
		}
	}

	cout << "Pedido no encontrado." << endl;
}

void Cafeteria::entregarPedido(const string& folio) {
	for(auto& p : listaPedidos) {
		if(p.getFolio() == folio) {
			p.cambiarEstado("Entregado");
			cout << "Pedido " << folio << " entregado." << endl;
			return;
		}
	}

	cout << "Pedido no encontrado." << endl;
}

void Cafeteria::gestionarPedido(const string& folio) {
	elaborarPedido(folio);
	entregarPedido(folio);
}

void Cafeteria::escanearQR(const string& codigo) {
	cout << "Codigo QR recibido: " << codigo << endl;
}

void Cafeteria::verificarPago(const string& folio) {
	BaseDatos db;

	if(db.conectar()) {
		Pago pago = db.obtenerPago(folio);

		cout << "Pago consultado para el pedido " << folio
		     << " -> " << (pago.getAprobado() ? "Aprobado" : "No aprobado") << endl;

		db.desconectar();
	}
}

void Cafeteria::atenderCajas(const Producto& producto, int cantidad) {
	float ganancia = producto.getPrecio() * cantidad;
	gananciaCajaTurno += ganancia;
	vendidosCajaTurno.push_back(producto);

	cout << "Atendiendo cajas.... +$" << ganancia << " [" << producto.getNombreProducto() << "]" << endl;
}
