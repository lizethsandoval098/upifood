#include "Administrador.h"
#include "Senales.h"
#include "Monitor.h"
#ifdef UPIIFOOD_CON_SFML
#include "VistaEnVivo.h"
#endif

#include <iostream>

using namespace std;

int main() {
    Senales::instalarCliente();   // Ctrl+C / SIGTERM -> salir ordenadamente

    Administrador admin;

    if (!admin.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale (o UPIIFOOD_IP) y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        if (Senales::interrumpido()) return 0;

        if (admin.iniciarSesionAdmin()) {
            autenticado = true;
        } else if (!cin) {
            return 0;    // EOF
        } else {
            cout << "Intenta de nuevo (o Ctrl+C para salir)." << endl;
        }
    }

    cout << "\nBienvenido/a, " << admin.getNombre() << endl;

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU ADMINISTRADOR -------------------" << endl;
        cout << "1) Ver usuarios (orden de registro)" << endl;
        cout << "2) Ver cafeterias (con cantidad de pedidos)" << endl;
        cout << "3) Ver usuarios en linea" << endl;
        cout << "4) Estadisticas (matrices: stock, ventas y pedidos por estado)" << endl;
        cout << "5) MONITOR EN VIVO en consola (todo lo que ocurre en el servidor)" << endl;
#ifdef UPIIFOOD_CON_SFML
        cout << "6) Abrir ventana en vivo (SFML)" << endl;
#endif
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: " << flush;

        int opcion;
        if (!Senales::leerEntero(opcion)) break;   // EOF o Ctrl+C

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
                admin.verEstadisticas();
                break;
            case 5:
                monitorEnVivoConsola("MONITOR ADMINISTRADOR");
                break;
#ifdef UPIIFOOD_CON_SFML
            case 6:
                abrirVentanaEnVivo("UPIIFOOD - Administrador", "Panel de Administrador");
                break;
#endif
            case 0:
                salir = true;
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    }

    cout << "\nHasta luego." << endl;
    return 0;
}
