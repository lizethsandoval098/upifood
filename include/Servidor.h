#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <vector>
#include <mutex>

#include "BaseDatos.h"

using namespace std;

class Servidor {

private:

    int socketServidor;
    int puerto;

    BaseDatos db;

    mutex dbMutex;

    // Usuarios conectados al servidor
    vector<string> usuariosEnLinea;

    // Protege la lista de usuarios conectados
    mutex mutexUsuariosEnLinea;

    void atenderCliente(int socketCliente);

    string procesarComando(
        const string& comando,
        int socketCliente
    );

    void agregarUsuarioEnLinea(
        const string& username
    );

    void eliminarUsuarioEnLinea(
        const string& username
    );

    vector<string> obtenerUsuariosEnLinea();

public:

    Servidor();

    bool iniciar();
};

#endif
