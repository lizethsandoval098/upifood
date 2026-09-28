#include "Administrador.h"
#include "Ventana.h"

#include <iostream>
#include <atomic>
#include <chrono>
#include <map>
#include <thread>

using namespace std;

int main() {
    Administrador admin;

    if (!admin.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        if (admin.iniciarSesionAdmin()) {
            autenticado = true;
        } else {
            cout << "Intenta de nuevo (o cierra la ventana con Ctrl+C)." << endl;
        }
    }

    cout << "\nBienvenido/a, " << admin.getNombre() << endl;

    Ventana ventana("UPIIFOOD - Administrador", 800, 600);
    ventana.agregarTexto("Panel de Administrador", 20, 20, 32);

    atomic<bool> monitorActivo{true};
    thread monitorPedidos([&admin, &ventana, &monitorActivo]() {
        map<string, string> estadosObservados;
        while (monitorActivo) {
            admin.cargarPedidos();
            vector<Pedido> pedidos = admin.getListaPedidos();
            vector<string> lineasVentana;
            for (const Pedido& pedido : pedidos) {
                auto anterior = estadosObservados.find(pedido.getFolio());
                if (anterior == estadosObservados.end() || anterior->second != pedido.getEstado()) {
                    cout << "\n[PANEL ADMIN] Pedido " << pedido.getFolio()
                         << " | cafe " << pedido.getIdCafeteria()
                         << " | cliente " << pedido.getUsernameCliente()
                         << " | " << pedido.getEstado() << " | $" << pedido.getTotal() << endl;
                    estadosObservados[pedido.getFolio()] = pedido.getEstado();
                }
                if (lineasVentana.size() < 15) {
                    lineasVentana.push_back("#" + pedido.getFolio() + "  Cafe " +
                        pedido.getIdCafeteria() + "  " + pedido.getEstado() + "  $" +
                        to_string(pedido.getTotal()));
                }
            }
            if (lineasVentana.empty()) lineasVentana.push_back("Esperando pedidos...");
            ventana.establecerLineasDinamicas(lineasVentana);
            for (int i = 0; i < 10 && monitorActivo; ++i)
                this_thread::sleep_for(chrono::milliseconds(200));
        }
    });

    // Mostrar el panel al iniciar sesión; el hilo monitor actualiza las líneas
    // y SFML solo dibuja desde el hilo principal.
    ventana.loop();

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU ADMINISTRADOR -------------------" << endl;
        cout << "1) Ver usuarios (orden de registro)" << endl;
        cout << "2) Ver cafeterias (con cantidad de pedidos)" << endl;
        cout << "3) Ver usuarios en linea" << endl;
        cout << "4) Ver pedidos ahora" << endl;
        cout << "5) Abrir ventana (SFML)" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: ";

        int opcion;
        cin >> opcion;

        switch (opcion) {
            case 1:
                admin.cargarUsuarios();
                admin.verUsuarios();
                break;
            case 2:
                admin.verCafeterias();
                break;
            case 3:
                admin.verUsuariosEnLinea();
                break;
            case 4:
                admin.cargarPedidos();
                admin.verPedidos();
                break;
            case 5:
                ventana.loop();
                break;
            case 0:
                salir = true;
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    }

    monitorActivo = false;
    if (monitorPedidos.joinable()) monitorPedidos.join();

    return 0;
}
