#include "Protocolo.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace std;

static const size_t MAX_LONGITUD_MENSAJE = 16384;

bool enviarMensaje(int socket, const string& mensaje) {
	string aEnviar = mensaje + "\n";

	size_t totalEnviado = 0;

	while (totalEnviado < aEnviar.size()) {
		// MSG_NOSIGNAL: si el otro lado ya cerro, send() regresa error en lugar
		// de matar el proceso con SIGPIPE (le pasaba al servidor si un cliente
		// se desconectaba justo cuando le estaba respondiendo).
		ssize_t enviado = send(socket, aEnviar.c_str() + totalEnviado,
		                        aEnviar.size() - totalEnviado, MSG_NOSIGNAL);

		if (enviado <= 0) {
			return false; // el socket se cerro o hubo error
		}

		totalEnviado += static_cast<size_t>(enviado);
	}

	return true;
}

bool recibirMensaje(int socket, string& mensajeRecibido) {
	mensajeRecibido.clear();

	char byte;

	while (true) {
		ssize_t leido = recv(socket, &byte, 1, 0);

		if (leido <= 0) {
			return false; // conexion cerrada, timeout o error
		}

		if (byte == '\n') {
			return true; // mensaje completo
		}

		if (byte != '\r') {
			mensajeRecibido += byte;
		}

		if (mensajeRecibido.size() > MAX_LONGITUD_MENSAJE) {
			return false; // mensaje absurdamente largo: se corta la conexion
		}
	}
}

string enviarComando(int socket, const string& comando) {
	if (!enviarMensaje(socket, comando)) {
		return "ERR|No se pudo enviar la peticion al servidor.";
	}

	string respuesta;

	if (!recibirMensaje(socket, respuesta)) {
		return "ERR|No se recibio respuesta del servidor.";
	}

	return respuesta;
}

vector<string> leerLineas(int socket, int cantidad) {
	vector<string> lineas;

	for (int i = 0; i < cantidad; i++) {
		string linea;

		if (!recibirMensaje(socket, linea)) {
			break;
		}

		lineas.push_back(linea);
	}

	return lineas;
}

vector<string> separarCampos(const string& mensaje, char separador) {
	vector<string> campos;
	string campo;

	for (char c : mensaje) {
		if (c == separador) {
			campos.push_back(campo);
			campo.clear();
		} else {
			campo += c;
		}
	}

	campos.push_back(campo);

	if (mensaje.empty()) {
		campos.clear();
	}

	return campos;
}

string obtenerIpServidor() {
	const char* ip = getenv("UPIIFOOD_IP");
	return (ip != nullptr && ip[0] != '\0') ? string(ip) : string("100.91.99.27");
}

int obtenerPuertoServidor() {
	const char* p = getenv("UPIIFOOD_PORT");

	if (p != nullptr && p[0] != '\0') {
		int valor = atoi(p);

		if (valor > 0 && valor < 65536) {
			return valor;
		}
	}

	return 5000;
}

int conectarAlServidor(const string& ip, int puerto, int timeoutSegundos) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1) {
		return -1;
	}

	sockaddr_in direccion;
	memset(&direccion, 0, sizeof(direccion));
	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(static_cast<uint16_t>(puerto));

	if (inet_pton(AF_INET, ip.c_str(), &direccion.sin_addr) != 1) {
		close(sock);
		return -1;
	}

	if (connect(sock, (sockaddr*)&direccion, sizeof(direccion)) == -1) {
		close(sock);
		return -1;
	}

	timeval tv;
	tv.tv_sec = timeoutSegundos;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	return sock;
}
