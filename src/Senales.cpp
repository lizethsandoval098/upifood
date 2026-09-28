#include "Senales.h"

#include <csignal>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <unistd.h>

using namespace std;

namespace {
	volatile sig_atomic_t g_interrumpido = 0;

	void manejadorTerminacion(int) {
		g_interrumpido = 1;
	}
}

namespace Senales {

	void instalarCliente() {
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = manejadorTerminacion;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0; // sin SA_RESTART a proposito (ver Senales.h)

		sigaction(SIGINT, &sa, nullptr);
		sigaction(SIGTERM, &sa, nullptr);

		signal(SIGPIPE, SIG_IGN);
	}

	bool interrumpido() {
		return g_interrumpido != 0;
	}

	void limpiar() {
		g_interrumpido = 0;
		cin.clear();
	}

	bool esperarEnterOInterrupcion() {
		while (!interrumpido()) {
			pollfd pfd;
			pfd.fd = STDIN_FILENO;
			pfd.events = POLLIN;
			pfd.revents = 0;

			int r = poll(&pfd, 1, 200);

			if (r > 0 && (pfd.revents & POLLIN)) {
				string basura;
				getline(cin, basura);
				return true;
			}

			if (r > 0 && (pfd.revents & (POLLHUP | POLLERR))) {
				return false; // stdin cerrado
			}
		}

		return false;
	}

	bool leerLinea(string& salida) {
		salida.clear();

		if (interrumpido()) {
			return false;
		}

		if (!getline(cin, salida)) {
			return false; // EOF o interrumpido por senal
		}

		string limpia;
		for (char c : salida) {
			if (c != '|' && c != '\r') {
				limpia += c;
			}
		}

		// recorta espacios de los extremos
		size_t ini = limpia.find_first_not_of(" \t");
		size_t fin = limpia.find_last_not_of(" \t");
		salida = (ini == string::npos) ? "" : limpia.substr(ini, fin - ini + 1);

		return true;
	}

	bool leerEntero(int& salida) {
		while (true) {
			string linea;

			if (!leerLinea(linea)) {
				return false;
			}

			if (linea.empty()) {
				continue;
			}

			try {
				size_t usados = 0;
				salida = stoi(linea, &usados);

				if (usados == linea.size()) {
					return true;
				}
			} catch (...) {
			}

			cout << "Escribe un numero valido: " << flush;
		}
	}
}
