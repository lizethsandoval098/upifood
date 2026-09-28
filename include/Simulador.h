#ifndef SIMULADOR_H
#define SIMULADOR_H

#include <string>

// =====================================================================
//  SIMULADOR DE CLIENTES  (para no tener que usar la web ni escribir a mano)
// =====================================================================
//  - fork()  : el proceso principal crea UN proceso hijo por cafeteria.
//  - hilos   : cada hijo lanza N hilos; cada hilo es un CLIENTE virtual con
//              su propio socket TCP que se registra, inicia sesion, arma
//              pedidos al azar, paga (a veces con tarjeta invalida), y espera
//              a que su pedido este listo para "recogerlo".
//  - pipe()  : cada hijo le manda su resumen final al padre por un pipe.
//  - senales : SIGINT/SIGTERM en el padre se reenvian a los hijos y estos
//              detienen a sus hilos; SIGUSR1 imprime el avance parcial.
//  - Al final revisa la CONSISTENCIA del inventario (no hubo condiciones de
//    carrera: stock_final == stock_inicial - unidades vendidas).
//
//  Mientras corre, la pantalla de Administrador / Cafeteria (opcion de
//  MONITOR EN VIVO) muestra todo lo que ocurre.
// =====================================================================

struct OpcionesSimulador {
    int clientesPorCafeteria = 6;
    int pedidosPorCliente = 3;
    int ritmoMs = 700;             // pausa promedio entre pedidos de un mismo cliente
    bool autoCafeteria = false;    // un hilo por cafeteria que prepara los pedidos solo
    int esperaEntregaMs = 0;       // cuanto espera cada cliente a que su pedido este "Listo"
    bool carrera = false;          // todos compiten por el MISMO producto (estres)
    bool soloUnaCafeteria = false;
    int cafeteriaUnica = 1;
    unsigned semilla = 0;          // 0 = usa la hora
    std::string ip;
    int puerto = 5000;
};

int ejecutarSimulacion(const OpcionesSimulador& opciones);

#endif
