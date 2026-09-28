#include "Cafeteria.h"
#include "BaseDatos.h"
#include "Pago.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <utility>

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
    if (socketCafeteria != -1) {
        close(socketCafeteria);
        socketCafeteria = -1;
    }
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
	lock_guard<mutex> guard(*mutexPedidos);
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
	lock_guard<mutex> guardSocket(*mutexSocket);
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
	vector<Pedido> pedidosActualizados;
	{
		lock_guard<mutex> guardSocket(*mutexSocket);
		string respuesta = enviarComando(socketCafeteria, "LISTAR_PEDIDOS_CAFETERIA");
		vector<string> encabezado = separarCampos(respuesta);
		if (encabezado.size() < 2 || encabezado[0] != "OK") return;

		int cantidad = 0;
		try {
			cantidad = stoi(encabezado[1]);
		} catch (...) {
			return;
		}

		for (int i = 0; i < cantidad; ++i) {
			string linea;
			if (!recibirMensaje(socketCafeteria, linea)) return;
			vector<string> campos = separarCampos(linea);
			if (campos.size() < 6) continue;

			Pedido pedido;
			pedido.setFolio(campos[0]);
			pedido.setFecha(campos[1]);
			pedido.setEstado(campos[2]);
			try {
				pedido.setTotal(stof(campos[3]));
			} catch (...) {
				continue;
			}
			pedido.setUsernameCliente(campos[4]);
			pedido.setIdCafeteria(campos[5]);
			pedidosActualizados.push_back(pedido);
		}
	}

	lock_guard<mutex> guardPedidos(*mutexPedidos);
	listaPedidos = move(pedidosActualizados);
}

void Cafeteria::restockProducto(const string& idProducto, int cantidad) {
    if (cantidad <= 0) {
        cout << "La cantidad debe ser mayor a cero." << endl;
        return;
    }

    string comando = "RESTOCK|" + idProducto + "|" + to_string(cantidad);
	string respuesta;
	{
		lock_guard<mutex> guardSocket(*mutexSocket);
		respuesta = enviarComando(socketCafeteria, comando);
	}
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

void Cafeteria::elaborarPedido(const string& folio) {
	if (actualizarEstadoPedido(folio, "Preparando"))
		cout << "Elaborando pedido: " << folio << endl;
	else
		cout << "No se pudo actualizar el pedido " << folio << "." << endl;
}

void Cafeteria::entregarPedido(const string& folio) {
	if (actualizarEstadoPedido(folio, "Entregado"))
		cout << "Pedido " << folio << " entregado." << endl;
	else
		cout << "No se pudo actualizar el pedido " << folio << "." << endl;
}

void Cafeteria::gestionarPedido(const string& folio) {
	if (!actualizarEstadoPedido(folio, "Preparando")) return;
	if (!actualizarEstadoPedido(folio, "Listo")) return;
	actualizarEstadoPedido(folio, "Entregado");
}

bool Cafeteria::actualizarEstadoPedido(const string& folio, const string& estado) {
	string respuesta;
	{
		lock_guard<mutex> guardSocket(*mutexSocket);
		respuesta = enviarComando(socketCafeteria,
			"ACTUALIZAR_PEDIDO|" + folio + "|" + estado);
	}
	vector<string> campos = separarCampos(respuesta);
	if (campos.empty() || campos[0] != "OK") return false;
	cargarListaPedidos();
	return true;
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
	string respuesta;
	{
		lock_guard<mutex> guardSocket(*mutexSocket);
		respuesta = enviarComando(socketCafeteria, "LOGIN_CAFETERIA|" + username + "|" + contrasena);
	}
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
