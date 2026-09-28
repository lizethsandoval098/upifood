#include "Simulador.h"
#include "Protocolo.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

static void uso(const char* prog) {
    cout << "Uso: " << prog << " [opciones]\n"
         << "  --clientes N        hilos-cliente por cafeteria (default 6)\n"
         << "  --pedidos N         pedidos por cliente (default 3)\n"
         << "  --ritmo MS          pausa entre pedidos en ms (default 700)\n"
         << "  --auto-cafeteria    un hilo por cafeteria prepara los pedidos automaticamente\n"
         << "  --entrega MS        cada cliente espera hasta MS a que su pedido este Listo y lo recoge\n"
         << "                      (con --auto-cafeteria el default es 6000)\n"
         << "  --carrera           prueba de estres: todos piden el MISMO producto casi sin pausa\n"
         << "  --cafeteria N       solo la cafeteria N (1 o 2); por defecto las dos, cada una en un fork()\n"
         << "  --semilla N         semilla aleatoria (para repetir una corrida)\n"
         << "  --ip A.B.C.D        IP del servidor (o variable UPIIFOOD_IP)\n"
         << "  --puerto N          puerto del servidor (o variable UPIIFOOD_PORT)\n";
}

int main(int argc, char* argv[]) {
    OpcionesSimulador op;
    op.ip = obtenerIpServidor();
    op.puerto = obtenerPuertoServidor();

    bool entregaDefinida = false;

    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        auto siguiente = [&](int& destino) {
            if (i + 1 >= argc) { uso(argv[0]); exit(1); }
            destino = atoi(argv[++i]);
        };

        if (a == "--clientes") siguiente(op.clientesPorCafeteria);
        else if (a == "--pedidos") siguiente(op.pedidosPorCliente);
        else if (a == "--ritmo") siguiente(op.ritmoMs);
        else if (a == "--entrega") { siguiente(op.esperaEntregaMs); entregaDefinida = true; }
        else if (a == "--auto-cafeteria") op.autoCafeteria = true;
        else if (a == "--carrera") op.carrera = true;
        else if (a == "--cafeteria") { siguiente(op.cafeteriaUnica); op.soloUnaCafeteria = true; }
        else if (a == "--semilla") { int s = 0; siguiente(s); op.semilla = static_cast<unsigned>(s); }
        else if (a == "--ip") { if (i + 1 >= argc) { uso(argv[0]); return 1; } op.ip = argv[++i]; }
        else if (a == "--puerto") siguiente(op.puerto);
        else { uso(argv[0]); return 1; }
    }

    if (op.clientesPorCafeteria < 1 || op.clientesPorCafeteria > 100 ||
        op.pedidosPorCliente < 1 || op.pedidosPorCliente > 1000 ||
        (op.soloUnaCafeteria && op.cafeteriaUnica != 1 && op.cafeteriaUnica != 2)) {
        cout << "Valores fuera de rango." << endl;
        uso(argv[0]);
        return 1;
    }

    if (op.autoCafeteria && !entregaDefinida) op.esperaEntregaMs = 6000;
    if (op.carrera && op.ritmoMs == 700) op.ritmoMs = 40;

    return ejecutarSimulacion(op);
}
