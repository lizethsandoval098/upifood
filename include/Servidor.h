#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <mutex>

#include "BaseDatos.h"

using namespace std;

class Servidor {

private:

    int socketServidor;
    int puerto;

    BaseDatos db;

    mutex dbMutex;

    void atenderCliente(int socketCliente);

    string procesarComando(
        const string& comando,
        int socketCliente
    );

public:

    Servidor();

    bool iniciar();
};

#endif
