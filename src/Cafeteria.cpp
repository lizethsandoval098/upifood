#include "Cafeteria.h"
#include "BaseDatos.h"
#include "Pago.h"
#include "Protocolo.h"

#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

using namespace std;

Cafeteria::Cafeteria() : gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = "100.70.231.3"; // IP de Tailscale de la compu con la BD/servidor
	puerto = 5000;
}

Cafeteria::Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria)
	: Usuario("Cafe", nombre, correo, contrasena, username),
	  idCafeteria{idCafeteria}, gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = "100.70.231.3";
	puerto = 5000;
}

// La copia copia los datos pero NO el socket (si no, el socket se cerraria
// dos veces: una por cada copia al destruirse).
Cafeteria::Cafeteria(const Cafeteria& otra)
	: Usuario(otra),
	  idCafeteria(otra.idCafeteria),
	  inventario(otra.inventario),
	  listaPedidos(otra.listaPedidos),
	  gananciaCajaTurno(otra.gananciaCajaTurno),
	  vendidosCajaTurno(otra.vendidosCajaTurno),
	  historialCaja(otra.historialCaja),
	  folioDetalle(otra.folioDetalle),
	  detallePedido(otra.detallePedido),
	  socketCafeteria(-1),
	  ipServidor(otra.ipServidor),
	  puerto(otra.puerto),
	  ultimoError(otra.ultimoError) {
}

Cafeteria& Cafeteria::operator=(const Cafeteria& otra) {
	if (this != &otra) {
		Usuario::operator=(otra);
		idCafeteria = otra.idCafeteria;
		inventario = otra.inventario;
		listaPedidos = otra.listaPedidos;
		gananciaCajaTurno = otra.gananciaCajaTurno;
		vendidosCajaTurno = otra.vendidosCajaTurno;
		historialCaja = otra.historialCaja;
		folioDetalle = otra.folioDetalle;
		detallePedido = otra.detallePedido;
		ipServidor = otra.ipServidor;
		puerto = otra.puerto;
		ultimoError = otra.ultimoError;
		// socketCafeteria NO se toca: cada objeto conserva el suyo.
	}
	return *this;
}

Cafeteria::~Cafeteria() {
	if (socketCafeteria != -1) {
		close(socketCafeteria);
		socketCafeteria = -1;
	}
}

void Cafeteria::setIpServidor(const string& ip) {
	ipServidor = ip;
}

bool Cafeteria::conectar() {
	socketCafeteria = socket(AF_INET, SOCK_STREAM, 0);

	if (socketCafeteria == -1) {
		ultimoError = "Error creando el socket.";
		cout << ultimoError << endl;
		return false;
	}

	sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_port = htons(puerto);

	if (inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr) != 1) {
		ultimoError = "La IP del servidor no es valida: " + ipServidor;
		cout << ultimoError << endl;
		close(socketCafeteria);
		socketCafeteria = -1;
		return false;
	}

	if (connect(socketCafeteria, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
		ultimoError = "No se pudo conectar a " + ipServidor + ". Revisa Tailscale y que 'servidor' este corriendo.";
		cout << ultimoError << endl;
		close(socketCafeteria);
		socketCafeteria = -1;
		return false;
	}

	// Si el servidor no contesta en 5 s, recv() falla en vez de quedarse
	// esperando para siempre (y congelar la ventana).
	timeval limite;
	limite.tv_sec = 5;
	limite.tv_usec = 0;
	setsockopt(socketCafeteria, SOL_SOCKET, SO_RCVTIMEO, &limite, sizeof(limite));

	cout << "Conectado al servidor (" << ipServidor << ")." << endl;
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

const vector<VentaCaja>& Cafeteria::getHistorialCaja() const {
	return historialCaja;
}

const vector<pair<Producto, int>>& Cafeteria::getDetallePedido() const {
	return detallePedido;
}

string Cafeteria::getFolioDetalle() const {
	return folioDetalle;
}

string Cafeteria::getUltimoError() const {
	return ultimoError;
}

void Cafeteria::setIdCafeteria(const string& id) {
	if(id == "1" || id == "2") {
		idCafeteria = id;
	}
	else {
		cout << "ID de cafeteria no valido." << endl;
	}
}

// ---------------------------------------------------------------------
// Peticiones de lista: el servidor contesta "OK|N" y luego N lineas mas.
// ---------------------------------------------------------------------
bool Cafeteria::pedirLista(const string& comando, vector<string>& lineas) {
	lineas.clear();

	string respuesta = enviarComando(socketCafeteria, comando);
	vector<string> encabezado = separarCampos(respuesta);

	if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
		if (!encabezado.empty() && encabezado[0] == "ERR" && encabezado.size() > 1) {
			ultimoError = encabezado[1];
		} else {
			ultimoError = "Respuesta invalida del servidor.";
		}
		return false;
	}

	int cantidad = 0;

	try {
		cantidad = stoi(encabezado[1]);
	} catch (...) {
		ultimoError = "Respuesta invalida del servidor.";
		return false;
	}

	for (int i = 0; i < cantidad; i++) {
		string linea;

		if (!recibirMensaje(socketCafeteria, linea)) {
			ultimoError = "Se perdio la conexion con el servidor.";
			return false;
		}

		lineas.push_back(linea);
	}

	return true;
}

// ---------------------------------------------------------------------
// Cargas
// ---------------------------------------------------------------------
bool Cafeteria::cargarInventario() {
	vector<string> lineas;

	if (!pedirLista("INVENTARIO|" + idCafeteria, lineas)) {
		return false;
	}

	inventario.clear();

	for (const string& linea : lineas) {
		// idProducto|nombre|stock|precio
		vector<string> campos = separarCampos(linea);

		if (campos.size() < 4) {
			continue;
		}

		Producto producto;
		producto.setIdProducto(campos[0]);
		producto.setNombreProducto(campos[1]);

		try {
			producto.setStock(stoi(campos[2]));
			producto.setPrecio(stof(campos[3]));
		} catch (...) {
			continue;
		}

		inventario.push_back(producto);
	}

	// Ordenado por ID (C1-01, C1-02, ...) para que la tabla no "baile".
	sort(inventario.begin(), inventario.end(), [](const Producto& a, const Producto& b) {
		return a.getIdProducto() < b.getIdProducto();
	});

	ultimoError.clear();
	return true;
}

bool Cafeteria::cargarListaPedidos() {
	vector<string> lineas;

	if (!pedirLista("PEDIDOS_CAFETERIA|" + idCafeteria, lineas)) {
		return false;
	}

	listaPedidos.clear();

	for (const string& linea : lineas) {
		// folio|username|estado|total|fecha
		vector<string> campos = separarCampos(linea);

		if (campos.size() < 5) {
			continue;
		}

		Pedido pedido;
		pedido.setFolio(campos[0]);
		pedido.setUsernameCliente(campos[1]);
		pedido.setEstado(campos[2]);

		try {
			pedido.setTotal(stof(campos[3]));
		} catch (...) {
			pedido.setTotal(0.0f);
		}

		pedido.setFecha(campos[4]);
		pedido.setIdCafeteria(idCafeteria);

		listaPedidos.push_back(pedido);
	}

	ultimoError.clear();
	return true;
}

bool Cafeteria::cargarDetallePedido(const string& folio) {
	vector<string> lineas;

	if (!pedirLista("PEDIDO_DETALLE|" + idCafeteria + "|" + folio, lineas)) {
		return false;
	}

	detallePedido.clear();
	folioDetalle = folio;

	for (const string& linea : lineas) {
		// idProducto|nombre|cantidad|precioUnitario
		vector<string> campos = separarCampos(linea);

		if (campos.size() < 4) {
			continue;
		}

		Producto producto;
		producto.setIdProducto(campos[0]);
		producto.setNombreProducto(campos[1]);

		int cantidad = 0;

		try {
			cantidad = stoi(campos[2]);
			producto.setPrecio(stof(campos[3]));
		} catch (...) {
			continue;
		}

		detallePedido.push_back({producto, cantidad});
	}

	ultimoError.clear();
	return true;
}

// ---------------------------------------------------------------------
// Acciones
// ---------------------------------------------------------------------
bool Cafeteria::restockProducto(const string& idProducto, int cantidad) {
	if (cantidad <= 0) {
		ultimoError = "La cantidad debe ser mayor a cero.";
		return false;
	}

	string respuesta = enviarComando(socketCafeteria, "RESTOCK|" + idProducto + "|" + to_string(cantidad));
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo actualizar el inventario.";
		return false;
	}

	// Respuesta esperada: OK|idProducto|nuevoStock
	if (campos.size() < 3) {
		ultimoError = "Respuesta invalida del servidor.";
		return false;
	}

	for (auto& producto : inventario) {
		if (producto.getIdProducto() == campos[1]) {
			try {
				producto.setStock(stoi(campos[2]));
			} catch (...) {
			}
			break;
		}
	}

	ultimoError.clear();
	return true;
}

bool Cafeteria::cambiarEstadoPedido(const string& folio, const string& nuevoEstado) {
	string respuesta = enviarComando(socketCafeteria,
	                                 "CAMBIAR_ESTADO|" + idCafeteria + "|" + folio + "|" + nuevoEstado);
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo cambiar el estado del pedido.";
		return false;
	}

	for (auto& p : listaPedidos) {
		if (p.getFolio() == folio) {
			p.cambiarEstado(nuevoEstado);
			break;
		}
	}

	ultimoError.clear();
	return true;
}

bool Cafeteria::elaborarPedido(const string& folio) {
	return cambiarEstadoPedido(folio, "Preparando");
}

bool Cafeteria::marcarListo(const string& folio) {
	return cambiarEstadoPedido(folio, "Listo");
}

bool Cafeteria::entregarPedido(const string& folio) {
	return cambiarEstadoPedido(folio, "Entregado");
}

bool Cafeteria::cancelarPedido(const string& folio) {
	return cambiarEstadoPedido(folio, "Cancelado");
}

bool Cafeteria::venderEnCaja(const string& idProducto, int cantidad) {
	if (cantidad <= 0) {
		ultimoError = "La cantidad debe ser mayor a cero.";
		return false;
	}

	string respuesta = enviarComando(socketCafeteria, "VENTA_CAJA|" + idProducto + "|" + to_string(cantidad));
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo registrar la venta.";
		return false;
	}

	// Respuesta esperada: OK|idProducto|nuevoStock|precioUnitario
	if (campos.size() < 4) {
		ultimoError = "Respuesta invalida del servidor.";
		return false;
	}

	int nuevoStock = 0;
	float precio = 0.0f;

	try {
		nuevoStock = stoi(campos[2]);
		precio = stof(campos[3]);
	} catch (...) {
		ultimoError = "Respuesta invalida del servidor.";
		return false;
	}

	string nombre = idProducto;

	for (auto& producto : inventario) {
		if (producto.getIdProducto() == campos[1]) {
			producto.setStock(nuevoStock);
			nombre = producto.getNombreProducto();
			vendidosCajaTurno.push_back(producto);
			break;
		}
	}

	float subtotal = precio * cantidad;
	gananciaCajaTurno += subtotal;
	historialCaja.push_back({nombre, cantidad, subtotal});

	ultimoError.clear();
	return true;
}

// ---------------------------------------------------------------------
// Consola
// ---------------------------------------------------------------------
void Cafeteria::verInventario() {
	if (!cargarInventario()) {
		cout << "No se pudo cargar el inventario: " << ultimoError << endl;
		return;
	}

	if(inventario.empty()) {
		cout << "Inventario vacio." << endl;
		return;
	}

	for(const auto& p : inventario) {
		cout << "[" << p.getIdProducto() << "] " << p.getNombreProducto()
		     << " - $" << p.getPrecio() << " (stock: " << p.getStock() << ")" << endl;
	}
}

// (Version vieja: abre la BD directo. La ventana no la usa.)
void Cafeteria::verificarPago(const string& folio) {
	BaseDatos db;

	if(db.conectar()) {
		Pago pago = db.obtenerPago(folio);

		cout << "Pago consultado para el pedido " << folio
		     << " -> " << (pago.getAprobado() ? "Aprobado" : "No aprobado") << endl;

		db.desconectar();
	}
}

// ---------------------------------------------------------------------
// Login
// ---------------------------------------------------------------------
bool Cafeteria::iniciarSesionCafeteria(const string& username, const string& contrasena) {
	if (username.empty() || contrasena.empty()) {
		ultimoError = "Escribe tu usuario y tu contrasena.";
		return false;
	}

	// Peticion al servidor: LOGIN_CAFETERIA|username|contrasena
	string respuesta = enviarComando(socketCafeteria, "LOGIN_CAFETERIA|" + username + "|" + contrasena);
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "Error desconocido.";
		return false;
	}

	// Respuesta esperada: OK|nombre|correo|idCafeteria|username
	if (campos.size() < 5) {
		ultimoError = "Respuesta invalida del servidor.";
		return false;
	}

	setNombre(campos[1]);
	setCorreo(campos[2]);
	setIdCafeteria(campos[3]);
	setUsername(campos[4]);
	setTipoUsuario("Cafe");

	ultimoError.clear();
	return true;
}

bool Cafeteria::iniciarSesionCafeteria() {
	cout << "==================================" << endl;
	cout << "Inicio de sesion (Cafeteria)" << endl;

	string username, contrasena;

	cout << "Ingrese username: ";
	cin >> username;

	cout << "Ingrese contrasena: ";
	cin >> contrasena;

	if (!iniciarSesionCafeteria(username, contrasena)) {
		cout << ultimoError << endl;
		return false;
	}

	cout << "Inicio de sesion correcto." << endl;
	return true;
}
