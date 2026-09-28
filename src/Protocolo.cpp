#include "Protocolo.h"

#include <sys/socket.h>

// Sin esto, escribir en un socket que el otro lado ya cerro manda la senal
// SIGPIPE y el sistema MATA el programa sin avisar. Con MSG_NOSIGNAL, send()
// solo regresa error y nosotros lo manejamos.
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#include <sstream>
#include <iostream>

using namespace std;

bool enviarMensaje(int socket, const string& mensaje) {
	string aEnviar = mensaje + "\n";

	size_t totalEnviado = 0;

	while (totalEnviado < aEnviar.size()) {
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
			return false; // conexion cerrada o error
		}

		if (byte == '\n') {
			return true; // mensaje completo
		}

		mensajeRecibido += byte;
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

vector<string> separarCampos(const string& mensaje, char separador) {
	vector<string> campos;
	stringstream ss(mensaje);
	string campo;

	while (getline(ss, campo, separador)) {
		campos.push_back(campo);
	}

	return campos;
}
