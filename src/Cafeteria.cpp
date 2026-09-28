#include "Cafeteria.h"
#include "BaseDatos.h"
#include "Pago.h"
#include "Protocolo.h"

#include <algorithm>
#include <cstdio>
#include <cctype>
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
	  productosPendientes(otra.productosPendientes),
	  gananciaCajaTurno(otra.gananciaCajaTurno),
	  vendidosCajaTurno(otra.vendidosCajaTurno),
	  historialCaja(otra.historialCaja),
	  folioDetalle(otra.folioDetalle),
	  detallePedido(otra.detallePedido),
	  foliosPedidoContabilizados(otra.foliosPedidoContabilizados),
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
		productosPendientes = otra.productosPendientes;
		gananciaCajaTurno = otra.gananciaCajaTurno;
		vendidosCajaTurno = otra.vendidosCajaTurno;
		historialCaja = otra.historialCaja;
		folioDetalle = otra.folioDetalle;
		detallePedido = otra.detallePedido;
		foliosPedidoContabilizados = otra.foliosPedidoContabilizados;
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

bool Cafeteria::hayProductoAgotado() const {
	for (const auto& producto : inventario) {
		if (producto.getStock() == 0) {
			return true;
		}
	}
	return false;
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

const vector<ProductoNuevo>& Cafeteria::getProductosPendientes() const {
	return productosPendientes;
}

void Cafeteria::setIdCafeteria(const string& id) {
	// El ID viene de la base de datos ("1", "2", "C-01", ...): basta con que traiga un numero 1-9.
	if(numeroCafeteria(id) != 0) {
		idCafeteria = id;
	}
	else {
		cout << "ID de cafeteria no valido: \"" << id << "\"." << endl;
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

	// Suma a la ganancia del turno los pedidos que ya se entregaron desde la
	// ultima vez que se pidio esta lista (vengan de la web o del simulador).
	contabilizarPedidosCompletados();

	ultimoError.clear();
	return true;
}

// Un pedido pagado/completado (estado "Entregado") suma su total a la
// ganancia del turno UNA sola vez, sin importar cuantas veces se vuelva a
// pedir la lista de pedidos (se recuerda su folio en foliosPedidoContabilizados).
void Cafeteria::contabilizarPedidosCompletados() {
	for (const auto& pedido : listaPedidos) {
		if (pedido.getEstado() != "Entregado") {
			continue;
		}

		bool yaContado = false;

		for (const auto& folio : foliosPedidoContabilizados) {
			if (folio == pedido.getFolio()) {
				yaContado = true;
				break;
			}
		}

		if (yaContado) {
			continue;
		}

		foliosPedidoContabilizados.push_back(pedido.getFolio());
		gananciaCajaTurno += pedido.getTotal();
		historialCaja.push_back({"Pedido " + pedido.getFolio() + " (" + pedido.getUsernameCliente() + ")",
		                         1, pedido.getTotal()});
	}
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

bool Cafeteria::completarPedido(const string& folio) {
	// Pendiente -> Preparando -> Listo -> Entregado (uno por uno, como pide el servidor)
	string actual;

	for (const auto& p : listaPedidos) {
		if (p.getFolio() == folio) {
			actual = p.getEstado();
			break;
		}
	}

	if (actual.empty()) {
		ultimoError = "Pedido no encontrado.";
		return false;
	}

	if (actual != "Pendiente" && actual != "Preparando" && actual != "Listo") {
		ultimoError = "El pedido ya esta " + actual + ".";
		return false;
	}

	if (actual == "Pendiente" && !cambiarEstadoPedido(folio, "Preparando")) return false;
	if ((actual == "Pendiente" || actual == "Preparando") && !cambiarEstadoPedido(folio, "Listo")) return false;
	return cambiarEstadoPedido(folio, "Entregado");
}

// ---------------------------------------------------------------------
// Edicion del inventario
// ---------------------------------------------------------------------
static string recortarBordes(const string& texto) {
	size_t inicio = texto.find_first_not_of(" \t\r\n");
	if (inicio == string::npos) return "";
	size_t fin = texto.find_last_not_of(" \t\r\n");
	return texto.substr(inicio, fin - inicio + 1);
}

static string enMinusculas(string texto) {
	for (char& c : texto) {
		c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	}
	return texto;
}

// Revisa lo que escribio la persona ANTES de mandarlo al servidor.
// Regresa "" si esta bien; si no, el motivo.
static string validarProducto(const string& nombre, float precio, int stock) {
	if (nombre.empty()) return "Escribe el nombre del producto.";
	if (nombre.size() > 60) return "El nombre es demasiado largo (maximo 60 caracteres).";
	if (nombre.find('|') != string::npos) return "El nombre no puede llevar el caracter |";
	if (!(precio > 0.0f)) return "El precio debe ser mayor a 0.";
	if (precio > 99999.0f) return "El precio es demasiado grande.";
	if (stock < 0) return "Las existencias no pueden ser negativas.";
	if (stock > 99999) return "Las existencias son demasiadas (maximo 99999).";
	return "";
}

static string precioATexto(float precio) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.2f", precio);
	return buffer;
}

bool Cafeteria::agregarProductoPendiente(const string& nombre, float precio, int stock) {
	string limpio = recortarBordes(nombre);
	string error = validarProducto(limpio, precio, stock);

	if (error.empty() && stock < 1) {
		error = "Un producto nuevo debe tener al menos 1 existencia inicial (no puede ser 0).";
	}

	if (!error.empty()) {
		ultimoError = error;
		return false;
	}

	for (const auto& p : inventario) {
		if (enMinusculas(p.getNombreProducto()) == enMinusculas(limpio)) {
			ultimoError = "Ya existe un producto llamado \"" + limpio + "\".";
			return false;
		}
	}

	for (const auto& p : productosPendientes) {
		if (enMinusculas(p.nombre) == enMinusculas(limpio)) {
			ultimoError = "Ya agregaste \"" + limpio + "\" a la lista de nuevos.";
			return false;
		}
	}

	productosPendientes.push_back({limpio, precio, stock});
	ultimoError.clear();
	return true;
}

bool Cafeteria::modificarProductoPendiente(size_t indice, const string& nombre, float precio, int stock) {
	if (indice >= productosPendientes.size()) {
		ultimoError = "Ese producto nuevo ya no esta en la lista.";
		return false;
	}

	string limpio = recortarBordes(nombre);
	string error = validarProducto(limpio, precio, stock);

	if (error.empty() && stock < 1) {
		error = "Un producto nuevo debe tener al menos 1 existencia inicial (no puede ser 0).";
	}

	if (!error.empty()) {
		ultimoError = error;
		return false;
	}

	for (const auto& p : inventario) {
		if (enMinusculas(p.getNombreProducto()) == enMinusculas(limpio)) {
			ultimoError = "Ya existe un producto llamado \"" + limpio + "\".";
			return false;
		}
	}

	for (size_t i = 0; i < productosPendientes.size(); i++) {
		if (i != indice && enMinusculas(productosPendientes[i].nombre) == enMinusculas(limpio)) {
			ultimoError = "Ya agregaste \"" + limpio + "\" a la lista de nuevos.";
			return false;
		}
	}

	productosPendientes[indice] = {limpio, precio, stock};
	ultimoError.clear();
	return true;
}

bool Cafeteria::quitarProductoPendiente(size_t indice) {
	if (indice >= productosPendientes.size()) {
		return false;
	}

	productosPendientes.erase(productosPendientes.begin() + static_cast<long>(indice));
	return true;
}

int Cafeteria::subirProductosPendientes() {
	if (productosPendientes.empty()) {
		ultimoError = "No hay productos nuevos por subir.";
		return 0;
	}

	int subidos = 0;
	string primerError;
	vector<ProductoNuevo> siguenPendientes;

	for (const auto& nuevo : productosPendientes) {
		// PRODUCTO_AGREGAR|idCafeteria|nombre|precio|stock   (el ID lo genera el servidor)
		string respuesta = enviarComando(socketCafeteria,
		                                 "PRODUCTO_AGREGAR|" + idCafeteria + "|" + nuevo.nombre + "|" +
		                                 precioATexto(nuevo.precio) + "|" + to_string(nuevo.stock));
		vector<string> campos = separarCampos(respuesta);

		if (!campos.empty() && campos[0] == "OK") {
			subidos++;
		} else {
			siguenPendientes.push_back(nuevo);

			if (primerError.empty()) {
				string motivo = (campos.size() > 1) ? campos[1] : "Error desconocido.";
				primerError = nuevo.nombre + ": " + motivo;
			}
		}
	}

	productosPendientes = siguenPendientes;
	ultimoError = primerError;

	// Refresca la tabla con lo que YA quedo guardado (con sus IDs nuevos).
	if (subidos > 0) {
		string errorSubida = ultimoError;
		cargarInventario();
		if (!errorSubida.empty()) ultimoError = errorSubida;
	}

	return subidos;
}

bool Cafeteria::modificarProducto(const string& idProducto, const string& nombre, float precio, int stock) {
	string limpio = recortarBordes(nombre);
	string error = validarProducto(limpio, precio, stock);

	if (!error.empty()) {
		ultimoError = error;
		return false;
	}

	string respuesta = enviarComando(socketCafeteria,
	                                 "PRODUCTO_MODIFICAR|" + idCafeteria + "|" + idProducto + "|" + limpio + "|" +
	                                 precioATexto(precio) + "|" + to_string(stock));
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo modificar el producto.";
		return false;
	}

	for (auto& p : inventario) {
		if (p.getIdProducto() == idProducto) {
			p.setNombreProducto(limpio);
			p.setPrecio(precio);
			p.setStock(stock);
			break;
		}
	}

	ultimoError.clear();
	return true;
}

bool Cafeteria::eliminarProducto(const string& idProducto) {
	string respuesta = enviarComando(socketCafeteria,
	                                 "PRODUCTO_ELIMINAR|" + idCafeteria + "|" + idProducto);
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo eliminar el producto.";
		return false;
	}

	for (auto it = inventario.begin(); it != inventario.end(); ++it) {
		if (it->getIdProducto() == idProducto) {
			inventario.erase(it);
			break;
		}
	}

	ultimoError.clear();
	return true;
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

bool Cafeteria::cerrarCaja(int& pedidosCerrados, float& totalPedidosCerrados) {
	string respuesta = enviarComando(socketCafeteria, "CORTE_CAJA|" + idCafeteria);
	vector<string> campos = separarCampos(respuesta);

	pedidosCerrados = 0;
	totalPedidosCerrados = 0.0f;

	if (campos.empty() || campos[0] != "OK") {
		ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo cerrar la caja.";
		return false;
	}

	try {
		if (campos.size() > 1) pedidosCerrados = stoi(campos[1]);
		if (campos.size() > 2) totalPedidosCerrados = stof(campos[2]);
	} catch (...) {
	}

	// Empieza el turno siguiente: se reinicia todo lo acumulado en caja
	// (los pedidos ya "Entregado" quedaron archivados del lado del servidor).
	gananciaCajaTurno = 0.0f;
	vendidosCajaTurno.clear();
	historialCaja.clear();
	foliosPedidoContabilizados.clear();

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
