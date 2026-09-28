#include "Cliente.h"
#include "Senales.h"
#ifdef UPIIFOOD_CON_SFML
#include "Ventana.h"
#endif

#include <iostream>

using namespace std;

int main() {
    Senales::instalarCliente();   // Ctrl+C / SIGTERM -> salir ordenadamente

    Cliente cliente;

    if (!cliente.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale (o UPIIFOOD_IP) y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        if (Senales::interrumpido()) return 0;

        cout << "\n==================== UPIIFOOD ====================" << endl;
        cout << "1) Iniciar sesion" << endl;
        cout << "2) Registrarme (solo correo institucional IPN)" << endl;
        cout << "3) Continuar como invitado" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: " << flush;

        int opcion;
        if (!Senales::leerEntero(opcion)) return 0;

        if (opcion == 1) {
            if (cliente.iniciarSesionCliente()) {
                autenticado = true;
            }
        } else if (opcion == 2) {
            if (cliente.registrarNuevoCliente()) {
                cout << "Ahora inicia sesion con tu nuevo usuario." << endl;
            }
        } else if (opcion == 3) {
            string nombreCompleto;
            cout << "Nombre completo (para saber a quien entregar el pedido): " << flush;
            if (!Senales::leerLinea(nombreCompleto)) return 0;

            cliente.entrarComoInvitado(nombreCompleto);
            autenticado = true;
        } else if (opcion == 0) {
            return 0;
        } else {
            cout << "Opcion invalida." << endl;
        }
    }

    cout << "\nBienvenido/a, " << cliente.getNombre()
         << " (" << cliente.getTipoCliente() << ")" << endl;

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU CLIENTE -------------------" << endl;
        cout << "1) Hacer pedido (elegir cafeteria y productos)" << endl;
        cout << "2) Pagar pedido" << endl;
        cout << "3) Ver estado del pedido" << endl;
        cout << "4) Recoger pedido" << endl;
        cout << "5) Ver historial de pedidos" << endl;
#ifdef UPIIFOOD_CON_SFML
        cout << "6) Abrir ventana (SFML)" << endl;
#endif
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: " << flush;

        int opcion;
        if (!Senales::leerEntero(opcion)) break;   // EOF o Ctrl+C

        switch (opcion) {
            case 1: cliente.hacerPedido(); break;
            case 2: cliente.pagar(); break;
            case 3: cliente.verEstadoPedido(); break;
            case 4: cliente.recogerPedido(); break;
            case 5: cliente.verHistorialP(); break;
#ifdef UPIIFOOD_CON_SFML
            case 6: {
                Ventana ventana("UPIIFOOD - Cliente", 800, 600);
                ventana.agregarTexto("UPIIFOOD", 20, 20, 32);
                ventana.agregarTexto("Cliente: " + cliente.getNombre(), 20, 80);
                ventana.agregarTexto("Tipo: " + cliente.getTipoCliente(), 20, 110);
                ventana.loop();   // bloquea hasta cerrar la ventana
                break;
            }
#endif
            case 0: salir = true; break;
            default: cout << "Opcion invalida." << endl;
        }
    }

    cout << "\nHasta luego." << endl;
    return 0;
}
