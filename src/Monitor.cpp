#include "Monitor.h"
#include "Protocolo.h"
#include "Senales.h"

#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

MonitorEventos::MonitorEventos(const string& ip, int puerto, const string& filtroCafeteria)
    : ip(ip), puerto(puerto), filtroCafeteria(filtroCafeteria), corriendo(false) {
}

MonitorEventos::~MonitorEventos() {
    detener();
}

bool MonitorEventos::iniciar(function<void(const string&)> callback) {
    if (corriendo) return true;

    alEvento = callback;
    corriendo = true;
    hilo = thread(&MonitorEventos::ciclo, this);
    return true;
}

void MonitorEventos::detener() {
    corriendo = false;

    if (hilo.joinable()) {
        hilo.join();
    }
}

void MonitorEventos::ciclo() {
    int sock = -1;
    long ultimoId = -1;   // -1 = la primera vez pide los ultimos eventos como contexto

    while (corriendo) {
        if (sock == -1) {
            sock = conectarAlServidor(ip, puerto, 5);

            if (sock == -1) {
                alEvento("[monitor] sin conexion con el servidor, reintentando...");

                for (int i = 0; i < 20 && corriendo; i++) {
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
                continue;
            }
        }

        string comando = "EVENTOS|" + to_string(ultimoId) + (filtroCafeteria.empty() ? "" : "|" + filtroCafeteria);
        string respuesta = enviarComando(sock, comando);
        vector<string> encabezado = separarCampos(respuesta);

        if (encabezado.size() < 3 || encabezado[0] != "OK") {
            close(sock);      // se reconecta en la siguiente vuelta
            sock = -1;
            continue;
        }

        int cantidad = 0;
        long ultimoServidor = 0;

        try {
            cantidad = stoi(encabezado[1]);
            ultimoServidor = stol(encabezado[2]);
        } catch (...) {
            continue;
        }

        vector<string> lineas = leerLineas(sock, cantidad);

        for (const string& linea : lineas) {
            // id|hora|tipo|idCafeteria|detalle
            vector<string> c = separarCampos(linea);
            if (c.size() < 5) continue;

            alEvento("[" + c[1] + "] " + c[2] + (c[3].empty() ? "" : " (caf " + c[3] + ")") + "  " + c[4]);
        }

        // Si el servidor se reinicio su contador vuelve a empezar: nos ajustamos.
        ultimoId = ultimoServidor;

        for (int i = 0; i < 10 && corriendo; i++) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    if (sock != -1) close(sock);
}

void monitorEnVivoConsola(const string& titulo, const string& filtroCafeteria) {
    cout << "\n=========== " << titulo << " ===========" << endl;
    cout << "Mostrando en vivo lo que pasa en el servidor." << endl;
    cout << "Presiona ENTER (o Ctrl+C) para volver al menu." << endl;
    cout << "-----------------------------------------------" << endl;

    MonitorEventos monitor(obtenerIpServidor(), obtenerPuertoServidor(), filtroCafeteria);

    // El callback corre en OTRO hilo: cada linea se arma completa y se manda
    // de un solo golpe para que no se mezcle con otra.
    monitor.iniciar([](const string& linea) {
        string salida = linea + "\n";
        cout << salida << flush;
    });

    Senales::esperarEnterOInterrupcion();

    monitor.detener();
    Senales::limpiar();   // Ctrl+C dentro del monitor solo regresa al menu

    cout << "-----------------------------------------------" << endl;
}
