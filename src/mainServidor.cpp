#include "../include/Servidor.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

using namespace std;

// Los manejadores de señales deben ser "async-signal-safe": no pueden usar
// mutex, cout, new/delete, etc. Por eso aqui SOLO se ponen banderas
// atomicas; el trabajo real (cerrar sockets, hacer fork, escribir
// archivos...) lo hace el hilo monitor de abajo, en un contexto normal.
static atomic<bool> banderaApagado{false};
static atomic<bool> banderaReporte{false};
static atomic<bool> banderaToggleSimulador{false};

extern "C" void manejarSenal(int senal) {
	if (senal == SIGINT || senal == SIGTERM) {
		banderaApagado.store(true);
	} else if (senal == SIGUSR1) {
		banderaReporte.store(true);
	} else if (senal == SIGUSR2) {
		banderaToggleSimulador.store(true);
	}
}

static void instalarManejadoresDeSenales() {
	struct sigaction accion;
	memset(&accion, 0, sizeof(accion));
	accion.sa_handler = manejarSenal;
	sigemptyset(&accion.sa_mask);
	accion.sa_flags = 0;

	sigaction(SIGINT, &accion, nullptr);
	sigaction(SIGTERM, &accion, nullptr);
	sigaction(SIGUSR1, &accion, nullptr);
	sigaction(SIGUSR2, &accion, nullptr);
}

int main(){
	Servidor servidor;

	instalarManejadoresDeSenales();

	cout << "Senales disponibles: SIGINT/SIGTERM = apagado seguro, "
	     << "SIGUSR1 = reporte de ventas (fork), SIGUSR2 = pausar/reanudar simulador." << endl;

	// Hilo monitor: revisa las banderas cada 200ms y ejecuta la accion real
	// (aqui SI se puede usar mutex, hacer I/O, fork(), lanzar hilos, etc.).
	thread hiloMonitor([&servidor]() {
		while (!banderaApagado.load()) {
			if (banderaReporte.exchange(false)) {
				servidor.generarReporteVentas();
			}

			if (banderaToggleSimulador.exchange(false)) {
				servidor.alternarSimulador();
			}

			this_thread::sleep_for(chrono::milliseconds(200));
		}

		servidor.detener();
	});

	servidor.iniciar(); // bloquea aceptando conexiones hasta que detener() cierre el socket

	hiloMonitor.join();

	return 0;
}
