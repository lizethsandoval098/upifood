#include "../include/Servidor.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

// Uso: servidor [--puerto N] [--seed]
//   --seed   inserta admin/cafeterias/productos de demostracion (si ya existen no pasa nada).
//            Si la base esta totalmente vacia se siembra sola.
int main(int argc, char* argv[]) {
    Servidor servidor;

    int puerto = 0;
    bool sembrar = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0) {
            sembrar = true;
        } else if (strcmp(argv[i], "--puerto") == 0 && i + 1 < argc) {
            puerto = atoi(argv[++i]);
        } else {
            cout << "Uso: " << argv[0] << " [--puerto N] [--seed]" << endl;
            return 1;
        }
    }

    servidor.configurar(puerto, sembrar);

    return servidor.iniciar() ? 0 : 1;
}
