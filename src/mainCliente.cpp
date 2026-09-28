#include "Cliente.h"
#include "Ventana.h"

#include <iostream>
#include <limits>

using namespace std;

void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {
    Cliente cliente;

    if (!cliente.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale y que 'servidor' este corriendo." << endl;
        return 1;
    }

    bool autenticado = false;

    while (!autenticado) {
        cout << "\n==================== UPIIFOOD ====================" << endl;
        cout << "1) Iniciar sesion" << endl;
        cout << "2) Registrarme (solo correo institucional IPN)" << endl;
        cout << "3) Continuar como invitado" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: ";

        int opcion;
        cin >> opcion;

        if (opcion == 1) {
            if (cliente.iniciarSesionCliente()) {
                autenticado = true;
            }
        } else if (opcion == 2) {
            if (cliente.registrarNuevoCliente()) {
                cout << "Ahora inicia sesion con tu nuevo usuario." << endl;
            }
        } else if (opcion == 3) {
            limpiarEntrada();
            string nombreCompleto;
            cout << "Nombre completo (para saber a quien entregar el pedido): ";
            getline(cin, nombreCompleto);

            cliente.setNombre(nombreCompleto);
            cliente.setTipoCliente("INVITADO");
            autenticado = true;
        } else if (opcion == 0) {
            return 0;
        } else {
            cout << "Opcion invalida." << endl;
        }
    }

    cout << "\nBienvenido/a, " << cliente.getNombre()
         << " (" << cliente.getTipoCliente() << ")" << endl;

    // Ventana SFML de referencia (requiere una fuente real en recursos/font.ttf)
    Ventana ventana("UPIIFOOD - Cliente", 800, 600);
    ventana.agregarTexto("UPIIFOOD", 20, 20, 32);
    ventana.agregarTexto("Cliente: " + cliente.getNombre(), 20, 80);
    ventana.agregarTexto("Tipo: " + cliente.getTipoCliente(), 20, 110);

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU CLIENTE -------------------" << endl;
        cout << "1) Ver / armar pedido" << endl;
        cout << "2) Pagar pedido" << endl;
        cout << "3) Ver estado del pedido" << endl;
        cout << "4) Recoger pedido" << endl;
        cout << "5) Ver historial de pedidos" << endl;
        cout << "6) Abrir ventana (SFML)" << endl;
        cout << "0) Salir" << endl;
        cout << "Elige una opcion: ";

        int opcion;
        cin >> opcion;

        switch (opcion) {
            case 1: cliente.hacerPedido(); break;
            case 2: cliente.pagar(); break;
            case 3: cliente.verEstadoPedido(); break;
            case 4: cliente.recogerPedido(); break;
            case 5: cliente.verHistorialP(); break;
            case 6: ventana.loop(); break; // bloquea hasta cerrar la ventana
            case 0: salir = true; break;
            default: cout << "Opcion invalida." << endl;
        }
    }

    return 0;
}
