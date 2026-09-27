#include "Servidor.h"
#include "Protocolo.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Usuario.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

Servidor::Servidor() {
	socketServidor = -1;
	puerto = 5000;
}

bool Servidor::iniciar() {
	// La base de datos se abre UNA sola vez, aqui, y se queda abierta todo
	// el tiempo que el servidor este corriendo (a diferencia de Cliente/
	// Cafeteria/Administrador, que ya no tocan la BD directo).
	if (!db.conectar()) {
		cout << "No se pudo abrir la base de datos. El servidor no puede iniciar." << endl;
		return false;
	}

	socketServidor = socket(AF_INET, SOCK_STREAM, 0);

	if (socketServidor == -1) {
		cout << "Error creando el socket del servidor." << endl;
		return false;
	}

	int opt = 1;
	setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in direccion;
	memset(&direccion, 0, sizeof(direccion));
	direccion.sin_family = AF_INET;
	direccion.sin_addr.s_addr = INADDR_ANY; // escucha en todas las interfaces (incluida Tailscale)
	direccion.sin_port = htons(puerto);

	if (bind(socketServidor, (sockaddr*)&direccion, sizeof(direccion)) == -1) {
		cout << "Error haciendo bind en el puerto " << puerto << "." << endl;
		return false;
	}

	if (listen(socketServidor, 10) == -1) {
		cout << "Error poniendo el socket en modo escucha." << endl;
		return false;
	}

	cout << "Servidor escuchando en el puerto " << puerto << "..." << endl;

	while (true) {
		sockaddr_in direccionCliente;
		socklen_t tam = sizeof(direccionCliente);

		int socketCliente = accept(socketServidor, (sockaddr*)&direccionCliente, &tam);

		if (socketCliente == -1) {
			cout << "Error aceptando una conexion." << endl;
			continue;
		}

		cout << "Nueva conexion aceptada." << endl;

		// Un hilo por cliente conectado: asi los 3 (cliente/cafeteria/admin)
		// pueden estar conectados y pidiendo cosas al mismo tiempo.
		thread hiloCliente(&Servidor::atenderCliente, this, socketCliente);
		hiloCliente.detach();
	}

	return true;
}

void Servidor::atenderCliente(int socketCliente) {
	while (true) {
		string comando;

		if (!recibirMensaje(socketCliente, comando)) {
			break; // el cliente se desconecto
		}

		cout << "Comando recibido: " << comando << endl;

		string respuesta = procesarComando(comando);

		if (!enviarMensaje(socketCliente, respuesta)) {
			break;
		}
	}

	close(socketCliente);
	cout << "Conexion cerrada." << endl;
}

string Servidor::procesarComando(const string& comando) {
	vector<string> campos = separarCampos(comando);

	if (campos.empty()) {
		return "ERR|Comando vacio.";
	}

	const string& tipo = campos[0];

	// ---------------------------------------------------------------
	// LOGIN_CLIENTE|username|contrasena
	// ---------------------------------------------------------------
	if (tipo == "LOGIN_CLIENTE") {
		if (campos.size() < 3) return "ERR|Formato invalido.";

		string username = campos[1];
		string contrasena = campos[2];

		lock_guard<mutex> guard(dbMutex); // seccion critica: acceso a la BD

		Cliente c = db.obtenerUsuarioCliente(username);

		if (c.getUsername().empty()) {
			return "ERR|Usuario inexistente.";
		}

		if (!c.verificarContrasena(contrasena)) {
			return "ERR|Contrasena incorrecta.";
		}

		return "OK|" + c.getNombre() + "|" + c.getCorreo() + "|" +
		       c.getApellidoPaterno() + "|" + c.getApellidoMaterno() + "|" +
		       c.getTipoCliente() + "|" + c.getUsername();
	}

	// ---------------------------------------------------------------
	// REGISTRO_CLIENTE|username|nombre|correo|contrasena|apellidoP|apellidoM|tipoCliente
	// ---------------------------------------------------------------
	if (tipo == "REGISTRO_CLIENTE") {
		if (campos.size() < 8) return "ERR|Formato invalido.";

		string username = campos[1];
		string nombre = campos[2];
		string correo = campos[3];
		string contrasena = campos[4];
		string apellidoP = campos[5];
		string apellidoM = campos[6];
		string tipoCliente = campos[7];

		Cliente nuevoCliente(nombre, correo, contrasena, username, apellidoP, apellidoM, tipoCliente);

		lock_guard<mutex> guard(dbMutex);

		bool exito = db.guardarUsuarioCliente(nuevoCliente);

		if (!exito) {
			return "ERR|No se pudo registrar (el username ya existe?).";
		}

		return "OK|" + username;
	}

	// ---------------------------------------------------------------
	// LOGIN_CAFETERIA|username|contrasena
	// ---------------------------------------------------------------
	if (tipo == "LOGIN_CAFETERIA") {
		if (campos.size() < 3) return "ERR|Formato invalido.";

		string username = campos[1];
		string contrasena = campos[2];

		lock_guard<mutex> guard(dbMutex);

		Cafeteria c = db.obtenerCafeteriaPorUsername(username);

		if (c.getUsername().empty()) {
			return "ERR|Usuario inexistente.";
		}

		if (!c.verificarContrasena(contrasena)) {
			return "ERR|Contrasena incorrecta.";
		}

		return "OK|" + c.getNombreCafeteria() + "|" + c.getCorreo() + "|" +
		       c.getIdCafeteria() + "|" + c.getUsername();
	}

	// ---------------------------------------------------------------
	// LOGIN_ADMIN|username|contrasena
	// ---------------------------------------------------------------
	if (tipo == "LOGIN_ADMIN") {
		if (campos.size() < 3) return "ERR|Formato invalido.";

		string username = campos[1];
		string contrasena = campos[2];

		lock_guard<mutex> guard(dbMutex);

		Usuario u = db.obtenerAdministrador(username);

		if (u.getUsername().empty()) {
			return "ERR|Usuario inexistente.";
		}

		if (!u.verificarContrasena(contrasena)) {
			return "ERR|Contrasena incorrecta.";
		}

		return "OK|" + u.getNombre() + "|" + u.getCorreo() + "|" + u.getUsername();
	}

	// TODO: siguientes comandos a agregar con el mismo patron:
	//   LISTAR_USUARIOS, LISTAR_CAFETERIAS, INVENTARIO|idCafeteria,
	//   RESTOCK|idProducto|cantidad, PEDIDOS_CAFETERIA|idCafeteria,
	//   GESTIONAR_PEDIDO|folio, GUARDAR_PEDIDO|..., etc.

	return "ERR|Comando desconocido: " + tipo;
}
