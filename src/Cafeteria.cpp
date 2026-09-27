#include "Cafeteria.h"
#include "BaseDatos.h"
#include "Pago.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

Cafeteria::Cafeteria() : gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = "100.91.99.27"; // IP de Tailscale de la compu con la BD/servidor
	puerto = 5000;
}

Cafeteria::Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria)
	: Usuario("Cafe", nombre, correo, contrasena, username),
	  idCafeteria{idCafeteria}, gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = "100.91.99.27";
	puerto = 5000;
}

Cafeteria::~Cafeteria() {
}

bool Cafeteria::conectar() {
	socketCafeteria = socket(AF_INET, SOCK_STREAM, 0);

	if (socketCafeteria == -1) {
		cout << "Error creando socket." << endl;
		return false;
	}

	sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_port = htons(puerto);
	inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr);

	if (connect(socketCafeteria, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
		cout << "Error conectando al servidor. Revisa la IP/Tailscale." << endl;
		return false;
	}

	cout << "Conectado al servidor correctamente." << endl;
	return true;
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

bool Cafeteria::iniciarSesionCafeteria() {
	cout << "==================================" << endl;
	cout << "Inicio de sesion (Cafeteria)" << endl;

	string username, contrasena;

	cout << "Ingrese username: ";
	cin >> username;

	cout << "Ingrese contrasena: ";
	cin >> contrasena;

	// Peticion al servidor: LOGIN_CAFETERIA|username|contrasena
	string respuesta = enviarComando(socketCafeteria, "LOGIN_CAFETERIA|" + username + "|" + contrasena);
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		string motivo = (campos.size() > 1) ? campos[1] : "Error desconocido.";
		cout << motivo << endl;
		return false;
	}

	// Respuesta esperada: OK|nombre|correo|idCafeteria|username
	if (campos.size() < 5) {
		cout << "Respuesta invalida del servidor." << endl;
		return false;
	}

	setNombre(campos[1]);
	setCorreo(campos[2]);
	setIdCafeteria(campos[3]);
	setUsername(campos[4]);
	setTipoUsuario("Cafe");

	cout << "Inicio de sesion correcto." << endl;
	return true;
}
