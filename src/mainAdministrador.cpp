#include "Administrador.h"
#include "Ventana.h"

#include <iostream>

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

    bool salir = false;

    while (!salir) {
        cout << "\n------------------- MENU ADMINISTRADOR -------------------" << endl;
        cout << "1) Ver usuarios (orden de registro)" << endl;
        cout << "2) Ver cafeterias (con cantidad de pedidos)" << endl;
        cout << "3) Ver usuarios en linea" << endl;
        cout << "4) Abrir ventana (SFML)" << endl;
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
                ventana.loop();
                break;
            case 0:
                salir = true;
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    }

    return 0;
}
