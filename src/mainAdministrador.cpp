#include "Administrador.h"
#include "Ventana.h"

#include <iostream>
#include <vector>

using namespace std;


void mostrarLista(
    Ventana& ventana,
    const string& titulo,
    const vector<string>& datos
) {

    ventana.limpiar();

    ventana.dibujarTexto(
        titulo,
        40,
        30,
        30
    );


    float posicionY = 90;


    for(const string& linea : datos) {

        ventana.dibujarTexto(
            linea,
            40,
            posicionY,
            18
        );

        posicionY += 35;

        /*
            Evitamos que demasiados registros
            se salgan de la pantalla.
        */

        if(posicionY > 650) {
            break;
        }
    }
}


int main()
{
    Administrador administrador;


    // =========================================================
    // CONEXION CON SERVIDOR
    // =========================================================

    cout << "====================================" << endl;
    cout << "       UPIIFOOD ADMINISTRADOR       " << endl;
    cout << "====================================" << endl;


    if(!administrador.conectar()) {

        cout
            << "No se pudo conectar al servidor."
            << endl;

        return 1;
    }


    // =========================================================
    // LOGIN
    // =========================================================

    bool autenticado = false;


    while(!autenticado) {

        autenticado =
            administrador.iniciarSesionAdmin();

        if(!autenticado) {

            cout
                << "Intenta nuevamente."
                << endl;
        }
    }


    cout << endl;

    cout
        << "Bienvenido/a "
        << administrador.getNombre()
        << endl;


    // =========================================================
    // CREAR VENTANA
    // =========================================================

    Ventana ventana(
        "UPIIFOOD - Administrador",
        1000,
        700
    );


    int pantalla = 0;


    /*
        0 = menu principal
        1 = usuarios
        2 = cafeterias
        3 = inventario
        4 = pedidos
        5 = usuarios conectados
    */


    vector<string> datos;


    // =========================================================
    // CICLO PRINCIPAL
    // =========================================================

    while(ventana.estaAbierta()) {


        sf::Event evento;


        while(ventana.obtenerEvento(evento)) {

            if(evento.type ==
               sf::Event::Closed) {

                ventana.cerrar();
            }


            if(
                evento.type ==
                sf::Event::MouseButtonPressed &&
                evento.mouseButton.button ==
                sf::Mouse::Left
            ) {


                float mouseX =
                    evento.mouseButton.x;

                float mouseY =
                    evento.mouseButton.y;


                // =================================================
                // MENU PRINCIPAL
                // =================================================

                if(pantalla == 0) {


                    // USUARIOS
                    if(
                        mouseX >= 50 &&
                        mouseX <= 300 &&
                        mouseY >= 120 &&
                        mouseY <= 180
                    ) {

                        pantalla = 1;

                        datos =
                            administrador
                                .obtenerUsuariosPanel();
                    }


                    // CAFETERIAS
                    else if(
                        mouseX >= 350 &&
                        mouseX <= 600 &&
                        mouseY >= 120 &&
                        mouseY <= 180
                    ) {

                        pantalla = 2;

                        datos =
                            administrador
                                .obtenerCafeteriasPanel();
                    }


                    // INVENTARIO
                    else if(
                        mouseX >= 650 &&
                        mouseX <= 900 &&
                        mouseY >= 120 &&
                        mouseY <= 180
                    ) {

                        pantalla = 3;

                        datos =
                            administrador
                                .obtenerInventarioPanel();
                    }


                    // PEDIDOS
                    else if(
                        mouseX >= 50 &&
                        mouseX <= 300 &&
                        mouseY >= 220 &&
                        mouseY <= 280
                    ) {

                        pantalla = 4;

                        datos =
                            administrador
                                .obtenerPedidosPanel();
                    }


                    // USUARIOS EN LINEA
                    else if(
                        mouseX >= 350 &&
                        mouseX <= 600 &&
                        mouseY >= 220 &&
                        mouseY <= 280
                    ) {

                        pantalla = 5;

                        datos =
                            administrador
                                .obtenerUsuariosEnLineaPanel();
                    }


                    // SALIR
                    else if(
                        mouseX >= 650 &&
                        mouseX <= 900 &&
                        mouseY >= 220 &&
                        mouseY <= 280
                    ) {

                        ventana.cerrar();
                    }
                }


                // =================================================
                // PANTALLAS INTERNAS
                // =================================================

                else {

                    // REGRESAR
                    if(
                        mouseX >= 40 &&
                        mouseX <= 220 &&
                        mouseY >= 620 &&
                        mouseY <= 675
                    ) {

                        pantalla = 0;
                    }


                    // ACTUALIZAR
                    else if(
                        mouseX >= 750 &&
                        mouseX <= 950 &&
                        mouseY >= 620 &&
                        mouseY <= 675
                    ) {

                        if(pantalla == 1) {

                            datos =
                                administrador
                                    .obtenerUsuariosPanel();
                        }

                        else if(pantalla == 2) {

                            datos =
                                administrador
                                    .obtenerCafeteriasPanel();
                        }

                        else if(pantalla == 3) {

                            datos =
                                administrador
                                    .obtenerInventarioPanel();
                        }

                        else if(pantalla == 4) {

                            datos =
                                administrador
                                    .obtenerPedidosPanel();
                        }

                        else if(pantalla == 5) {

                            datos =
                                administrador
                                    .obtenerUsuariosEnLineaPanel();
                        }
                    }
                }
            }
        }


        // =========================================================
        // DIBUJAR
        // =========================================================

        ventana.limpiar();


        // =========================================================
        // MENU PRINCIPAL
        // =========================================================

        if(pantalla == 0) {

            ventana.dibujarTexto(
                "UPIIFOOD",
                40,
                30,
                38
            );

            ventana.dibujarTexto(
                "Panel de Administrador",
                40,
                75,
                24
            );


            ventana.dibujarTexto(
                "Administrador: " +
                administrador.getNombre(),
                650,
                40,
                18
            );


            ventana.dibujarBoton(
                "USUARIOS",
                50,
                120,
                250,
                60
            );


            ventana.dibujarBoton(
                "CAFETERIAS",
                350,
                120,
                250,
                60
            );


            ventana.dibujarBoton(
                "INVENTARIO",
                650,
                120,
                250,
                60
            );


            ventana.dibujarBoton(
                "PEDIDOS",
                50,
                220,
                250,
                60
            );


            ventana.dibujarBoton(
                "USUARIOS EN LINEA",
                350,
                220,
                250,
                60
            );


            ventana.dibujarBoton(
                "SALIR",
                650,
                220,
                250,
                60
            );
        }


        // =========================================================
        // PANTALLAS DE INFORMACION
        // =========================================================

        else {

            string titulo;


            if(pantalla == 1) {
                titulo = "USUARIOS REGISTRADOS";
            }

            else if(pantalla == 2) {
                titulo = "CAFETERIAS";
            }

            else if(pantalla == 3) {
                titulo = "INVENTARIO";
            }

            else if(pantalla == 4) {
                titulo = "PEDIDOS";
            }

            else {
                titulo = "USUARIOS EN LINEA";
            }


            mostrarLista(
                ventana,
                titulo,
                datos
            );


            ventana.dibujarBoton(
                "REGRESAR",
                40,
                620,
                180,
                55
            );


            ventana.dibujarBoton(
                "ACTUALIZAR",
                750,
                620,
                200,
                55
            );
        }


        ventana.mostrar();
    }


    return 0;
}
