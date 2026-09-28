#include "Cafeteria.h"
#include "Ventana.h"

#include <iostream>

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

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU CAFETERIA -------------------" << endl;
        cout << "1) Ver inventario" << endl;
        cout << "2) Reabastecer producto" << endl;
        cout << "3) Ver pedidos pendientes" << endl;
        cout << "4) Gestionar (elaborar + entregar) un pedido por folio" << endl;
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
            cout << "Folio del pedido: ";
            cin >> folio;
            cafeteria.gestionarPedido(folio);
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

    return 0;
}
