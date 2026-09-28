#include "Cafeteria.h"
#include "Ventana.h"

#include <iostream>
#include <atomic>
#include <chrono>
#include <map>
#include <thread>

using namespace std;

int main() {
    Cafeteria cafeteria;

    if (!cafeteria.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        if (cafeteria.iniciarSesionCafeteria()) {
            autenticado = true;
        } else {
            cout << "Intenta de nuevo (o cierra la ventana con Ctrl+C)." << endl;
        }
    }

    cout << "\nBienvenido/a, cafeteria " << cafeteria.getNombreCafeteria()
         << " (ID " << cafeteria.getIdCafeteria() << ")" << endl;

    cafeteria.cargarInventario();
    cafeteria.cargarListaPedidos();

    Ventana ventana("UPIIFOOD - Cafeteria " + cafeteria.getIdCafeteria(), 800, 600);
    ventana.agregarTexto("Cafeteria " + cafeteria.getNombreCafeteria(), 20, 20, 32);

    atomic<bool> monitorActivo{true};
    thread monitorPedidos([&cafeteria, &ventana, &monitorActivo]() {
        map<string, string> estadosObservados;
        while (monitorActivo) {
            cafeteria.cargarListaPedidos();
            vector<Pedido> pedidos = cafeteria.getListaPedidos();
            vector<string> lineasVentana;
            for (const Pedido& pedido : pedidos) {
                auto anterior = estadosObservados.find(pedido.getFolio());
                if (anterior == estadosObservados.end() || anterior->second != pedido.getEstado()) {
                    cout << "\n[PANEL CAFETERIA] Pedido " << pedido.getFolio()
                         << " | cliente " << pedido.getUsernameCliente()
                         << " | " << pedido.getEstado() << " | $" << pedido.getTotal() << endl;
                    estadosObservados[pedido.getFolio()] = pedido.getEstado();
                }
                if (lineasVentana.size() < 15) {
                    lineasVentana.push_back("#" + pedido.getFolio() + "  " + pedido.getEstado() +
                        "  Cliente " + pedido.getUsernameCliente() + "  $" + to_string(pedido.getTotal()));
                }
            }
            if (lineasVentana.empty()) lineasVentana.push_back("Esperando pedidos...");
            ventana.establecerLineasDinamicas(lineasVentana);
            for (int i = 0; i < 10 && monitorActivo; ++i)
                this_thread::sleep_for(chrono::milliseconds(200));
        }
    });

    // La ventana se abre de inmediato; su bucle SFML sigue en el hilo principal
    // mientras el monitor de pedidos consulta al servidor en segundo plano.
    ventana.loop();

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU CAFETERIA -------------------" << endl;
        cout << "1) Ver inventario" << endl;
        cout << "2) Reabastecer producto" << endl;
        cout << "3) Ver todos los pedidos" << endl;
        cout << "4) Cambiar estado de pedido" << endl;
        cout << "5) Atender venta en caja" << endl;
        cout << "6) Abrir ventana (SFML)" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: ";

        int opcion;
        cin >> opcion;

        if (opcion == 1) {
            cafeteria.verInventario();
        } else if (opcion == 2) {
            string idProducto;
            int cantidad;
            cout << "ID de producto: ";
            cin >> idProducto;
            cout << "Cantidad a agregar: ";
            cin >> cantidad;
            cafeteria.restockProducto(idProducto, cantidad);
        } else if (opcion == 3) {
            cafeteria.cargarListaPedidos();
            for (const auto& p : cafeteria.getListaPedidos()) {
                cout << "Folio " << p.getFolio() << " - " << p.getEstado()
                     << " - $" << p.getTotal() << endl;
            }
        } else if (opcion == 4) {
            string folio;
            int estado;
            cout << "Folio del pedido: ";
            cin >> folio;
            cout << "Estado: 1) Preparando  2) Listo  3) Entregado  4) Cancelado: ";
            cin >> estado;
            const string estados[] = {"", "Preparando", "Listo", "Entregado", "Cancelado"};
            if (estado >= 1 && estado <= 4) {
                if (cafeteria.actualizarEstadoPedido(folio, estados[estado])) {
                    cout << "Estado actualizado a " << estados[estado] << "." << endl;
                } else {
                    cout << "No se pudo aplicar esa transicion de estado." << endl;
                }
            } else {
                cout << "Estado invalido." << endl;
            }
        } else if (opcion == 5) {
            string idProducto;
            int cantidad;
            cout << "ID de producto vendido en caja: ";
            cin >> idProducto;
            cout << "Cantidad: ";
            cin >> cantidad;

            bool encontrado = false;
            for (const auto& p : cafeteria.getInventario()) {
                if (p.getIdProducto() == idProducto) {
                    cafeteria.atenderCajas(p, cantidad);
                    encontrado = true;
                    break;
                }
            }
            if (!encontrado) {
                cout << "Producto no encontrado en el inventario." << endl;
            }
        } else if (opcion == 6) {
            ventana.loop();
        } else if (opcion == 0) {
            salir = true;
        } else {
            cout << "Opcion invalida." << endl;
        }
    }

    monitorActivo = false;
    if (monitorPedidos.joinable()) monitorPedidos.join();

    return 0;
}
