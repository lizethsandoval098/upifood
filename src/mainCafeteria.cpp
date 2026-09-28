#include "Cafeteria.h"
#include "Senales.h"
#include "Monitor.h"
#ifdef UPIIFOOD_CON_SFML
#include "VistaEnVivo.h"
#endif

#include <iostream>

using namespace std;

int main() {
    Senales::instalarCliente();   // Ctrl+C / SIGTERM -> salir ordenadamente

    Cafeteria cafeteria;

    if (!cafeteria.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale (o UPIIFOOD_IP) y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        if (Senales::interrumpido()) return 0;

        if (cafeteria.iniciarSesionCafeteria()) {
            autenticado = true;
        } else if (!cin) {
            return 0;    // EOF
        } else {
            cout << "Intenta de nuevo (o Ctrl+C para salir)." << endl;
        }
    }

    cout << "\nBienvenido/a, cafeteria " << cafeteria.getNombreCafeteria()
         << " (ID " << cafeteria.getIdCafeteria() << ")" << endl;

    cafeteria.cargarInventario();

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU CAFETERIA -------------------" << endl;
        cout << "1) Ver inventario" << endl;
        cout << "2) Reabastecer producto" << endl;
        cout << "3) Ver pedidos" << endl;
        cout << "4) Avanzar un pedido (Pendiente > Preparando > Listo > Entregado)" << endl;
        cout << "5) Atender venta en caja" << endl;
        cout << "6) MONITOR EN VIVO en consola (pedidos, pagos, estados...)" << endl;
#ifdef UPIIFOOD_CON_SFML
        cout << "7) Abrir ventana en vivo (SFML)" << endl;
#endif
        cout << "8) Cancelar un pedido" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: " << flush;

        int opcion;
        if (!Senales::leerEntero(opcion)) break;   // EOF o Ctrl+C

        if (opcion == 1) {
            cafeteria.cargarInventario();
            cafeteria.verInventario();
        } else if (opcion == 2) {
            string idProducto;
            int cantidad = 0;
            cout << "ID de producto: " << flush;
            if (!Senales::leerLinea(idProducto)) break;
            cout << "Cantidad a agregar: " << flush;
            if (!Senales::leerEntero(cantidad)) break;
            cafeteria.restockProducto(idProducto, cantidad);
        } else if (opcion == 3) {
            cafeteria.verPedidos();
        } else if (opcion == 4) {
            string folio;
            cout << "Folio del pedido: " << flush;
            if (!Senales::leerLinea(folio)) break;
            cafeteria.gestionarPedido(folio);
        } else if (opcion == 5) {
            string idProducto;
            int cantidad = 0;
            cout << "ID de producto vendido en caja: " << flush;
            if (!Senales::leerLinea(idProducto)) break;
            cout << "Cantidad: " << flush;
            if (!Senales::leerEntero(cantidad)) break;

            cafeteria.cargarInventario();

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
            monitorEnVivoConsola("MONITOR CAFETERIA " + cafeteria.getIdCafeteria(), cafeteria.getIdCafeteria());
#ifdef UPIIFOOD_CON_SFML
        } else if (opcion == 7) {
            abrirVentanaEnVivo("UPIIFOOD - Cafeteria " + cafeteria.getIdCafeteria(),
                               "Cafeteria " + cafeteria.getNombreCafeteria(), cafeteria.getIdCafeteria());
#endif
        } else if (opcion == 8) {
            string folio;
            cout << "Folio del pedido a cancelar: " << flush;
            if (!Senales::leerLinea(folio)) break;
            cafeteria.cancelarPedido(folio);
        } else if (opcion == 0) {
            salir = true;
        } else {
            cout << "Opcion invalida." << endl;
        }
    }

    cout << "\nHasta luego." << endl;
    return 0;
}
