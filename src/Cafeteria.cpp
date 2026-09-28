#include "Cafeteria.h"
#include "Protocolo.h"
#include "Senales.h"

#include <iomanip>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

Cafeteria::Cafeteria() : gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = obtenerIpServidor(); // por defecto la IP de Tailscale; cambia con UPIIFOOD_IP
	puerto = obtenerPuertoServidor();
}

Cafeteria::Cafeteria(string nombre, string correo, string contrasena, string username, string idCafeteria)
	: Usuario("Cafe", nombre, correo, contrasena, username),
	  idCafeteria{idCafeteria}, gananciaCajaTurno(0.0f) {
	socketCafeteria = -1;
	ipServidor = obtenerIpServidor();
	puerto = obtenerPuertoServidor();
}

Cafeteria::~Cafeteria() {
    if (socketCafeteria != -1) {
        close(socketCafeteria);
        socketCafeteria = -1;
    }
}

bool Cafeteria::conectar() {
	socketCafeteria = conectarAlServidor(ipServidor, puerto);

	if (socketCafeteria == -1) {
		cout << "Error conectando al servidor " << ipServidor << ":" << puerto << ". Revisa la IP/Tailscale." << endl;
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
    string respuesta = enviarComando(socketCafeteria, "INVENTARIO|" + idCafeteria);
    vector<string> encabezado = separarCampos(respuesta);

    inventario.clear();

    if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
        cout << "No se pudo cargar el inventario." << endl;
        if (encabezado.size() > 1) cout << encabezado[1] << endl;
        return;
    }

    int cantidad = 0;
    try {
        cantidad = stoi(encabezado[1]);
    } catch (...) {
        cout << "Respuesta invalida del servidor." << endl;
        return;
    }

    for (int i = 0; i < cantidad; i++) {
        string linea;

        if (!recibirMensaje(socketCafeteria, linea)) {
            cout << "No se recibieron todos los productos." << endl;
            return;
        }

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
	listaPedidos.clear();
	foliosPagados.clear();

	// LISTAR_PEDIDOS|idCaf -> OK|N y N lineas: folio|fecha|estado|total|username|pagado
	vector<string> encabezado = separarCampos(enviarComando(socketCafeteria, "LISTAR_PEDIDOS|" + idCafeteria));

	if (encabezado.size() < 2 || encabezado[0] != "OK") {
		cout << "No se pudo cargar la lista de pedidos." << endl;
		return;
	}

	int cantidad = 0;
	try { cantidad = stoi(encabezado[1]); } catch (...) { return; }

	for (const string& linea : leerLineas(socketCafeteria, cantidad)) {
		vector<string> c = separarCampos(linea);
		if (c.size() < 6) continue;

		Pedido p;
		p.setFolio(c[0]);
		p.setFecha(c[1]);
		p.setEstado(c[2]);
		try { p.setTotal(stof(c[3])); } catch (...) {}
		p.setUsernameCliente(c[4]);
		p.setIdCafeteria(idCafeteria);

		listaPedidos.push_back(p);
		if (c[5] == "1") foliosPagados.insert(c[0]);
	}
}

bool Cafeteria::estaPagado(const string& folio) const {
	return foliosPagados.count(folio) > 0;
}

void Cafeteria::verPedidos() {
	cargarListaPedidos();

	if (listaPedidos.empty()) {
		cout << "No hay pedidos registrados en esta cafeteria." << endl;
		return;
	}

	cout << left << setw(12) << "FOLIO" << setw(21) << "FECHA" << setw(12) << "ESTADO"
	     << setw(10) << "TOTAL" << setw(8) << "PAGADO" << "CLIENTE" << endl;

	for (const auto& p : listaPedidos) {
		cout << left << setw(12) << p.getFolio() << setw(21) << p.getFecha() << setw(12) << p.getEstado()
		     << "$" << setw(9) << fixed << setprecision(2) << p.getTotal()
		     << setw(8) << (estaPagado(p.getFolio()) ? "si" : "no")
		     << p.getUsernameCliente() << endl;
	}
}

void Cafeteria::restockProducto(const string& idProducto, int cantidad) {
    if (cantidad <= 0) {
        cout << "La cantidad debe ser mayor a cero." << endl;
        return;
    }

    string comando = "RESTOCK|" + idProducto + "|" + to_string(cantidad);
    string respuesta = enviarComando(socketCafeteria, comando);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        string motivo = (campos.size() > 1) ? campos[1] : "No se pudo actualizar el inventario.";
        cout << motivo << endl;
        return;
    }

    if (campos.size() < 3) {
        cout << "Respuesta invalida del servidor." << endl;
        return;
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

    cout << "Producto reabastecido correctamente." << endl;
}

// Pide al servidor cambiar el estado de un pedido. Es el SERVIDOR quien valida
// que la transicion sea legal (asi, si dos cafeterias o dos hilos lo intentan
// a la vez, solo uno gana y no hay estados imposibles).
static bool cambiarEstadoEnServidor(int socket, const string& folio, const string& nuevoEstado) {
	vector<string> r = separarCampos(enviarComando(socket, "CAMBIAR_ESTADO|" + folio + "|" + nuevoEstado));

	if (r.empty() || r[0] != "OK") {
		cout << (r.size() > 1 ? r[1] : "No se pudo cambiar el estado.") << endl;
		return false;
	}

	return true;
}

void Cafeteria::elaborarPedido(const string& folio) {
	if (cambiarEstadoEnServidor(socketCafeteria, folio, "Preparando")) {
		cout << "Elaborando pedido: " << folio << endl;
	}
}

void Cafeteria::marcarListo(const string& folio) {
	if (cambiarEstadoEnServidor(socketCafeteria, folio, "Listo")) {
		cout << "Pedido " << folio << " LISTO para recoger." << endl;
	}
}

void Cafeteria::entregarPedido(const string& folio) {
	if (cambiarEstadoEnServidor(socketCafeteria, folio, "Entregado")) {
		cout << "Pedido " << folio << " entregado." << endl;
	}
}

void Cafeteria::cancelarPedido(const string& folio) {
	if (cambiarEstadoEnServidor(socketCafeteria, folio, "Cancelado")) {
		cout << "Pedido " << folio << " cancelado (el stock se libero)." << endl;
	}
}

void Cafeteria::gestionarPedido(const string& folio) {
	vector<string> r = separarCampos(enviarComando(socketCafeteria, "ESTADO_PEDIDO|" + folio));

	if (r.size() < 6 || r[0] != "OK") {
		cout << "Pedido no encontrado." << endl;
		return;
	}

	const string& estado = r[2];

	if (estado == "Pendiente") elaborarPedido(folio);
	else if (estado == "Preparando") marcarListo(folio);
	else if (estado == "Listo") entregarPedido(folio);
	else cout << "El pedido " << folio << " ya esta en estado final (" << estado << ")." << endl;
}

void Cafeteria::verificarPago(const string& folio) {
	// ESTADO_PEDIDO|folio -> OK|folio|estado|total|pagado|idCaf|user
	vector<string> r = separarCampos(enviarComando(socketCafeteria, "ESTADO_PEDIDO|" + folio));

	if (r.size() < 6 || r[0] != "OK") {
		cout << "Pedido no encontrado." << endl;
		return;
	}

	cout << "Pago consultado para el pedido " << folio
	     << " -> " << (r[4] == "1" ? "Aprobado" : "No aprobado") << endl;
}

void Cafeteria::atenderCajas(const Producto& producto, int cantidad) {
	// VENTA_CAJA|idProducto|cantidad -> OK|idProducto|nuevoStock
	vector<string> r = separarCampos(enviarComando(socketCafeteria,
	                                 "VENTA_CAJA|" + producto.getIdProducto() + "|" + to_string(cantidad)));

	if (r.empty() || r[0] != "OK") {
		cout << (r.size() > 1 ? r[1] : "No se pudo registrar la venta.") << endl;
		return;
	}

	float ganancia = producto.getPrecio() * cantidad;
	gananciaCajaTurno += ganancia;
	vendidosCajaTurno.push_back(producto);

	for (auto& p : inventario) {
		if (p.getIdProducto() == producto.getIdProducto() && r.size() >= 3) {
			try { p.setStock(stoi(r[2])); } catch (...) {}
		}
	}

	cout << "Atendiendo cajas.... +$" << ganancia << " [" << producto.getNombreProducto() << "]" << endl;
}

bool Cafeteria::iniciarSesionCafeteria() {
	cout << "==================================" << endl;
	cout << "Inicio de sesion (Cafeteria)" << endl;

	string username, contrasena;

	cout << "Ingrese username: " << flush;
	if (!Senales::leerLinea(username)) return false;

	cout << "Ingrese contrasena: " << flush;
	if (!Senales::leerLinea(contrasena)) return false;

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
