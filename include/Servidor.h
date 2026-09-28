#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>

#include "BaseDatos.h"

using namespace std;

class Servidor {

private:
    int socketServidor;
    int puerto;
    struct HiloClienteControl {
        thread hilo;
        shared_ptr<atomic<bool>> terminado;
    };
    vector<HiloClienteControl> hilosClientes;
    vector<int> socketsClientes;
    mutex mutexSocketsClientes;

    // Una sola conexion a SQLite, compartida por los hilos del servidor.
    BaseDatos db;
    mutex dbMutex;

    // Usuarios conectados actualmente al servidor.
    unordered_map<string, size_t> conexionesUsuariosEnLinea;
    mutex mutexUsuariosEnLinea;

    // Matriz de inventario:
    // fila 0 -> cafeteria 1 (C1-01, C1-02, ...)
    // fila 1 -> cafeteria 2 (C2-01, C2-02, ...)
    // columna 0 -> producto 01, columna 1 -> producto 02, etc.
    static const int MAX_PRODUCTOS_CAFETERIA = 20;
    int inventario[2][MAX_PRODUCTOS_CAFETERIA];
    // Si una operación necesita ambas, siempre toma dbMutex antes de mutexInventario.
    mutex mutexInventario;

    void atenderCliente(int socketCliente, const string& ipCliente);
    string procesarComando(const string& comando, const string& usernameConectado,
                           const string& tipoUsuarioConectado, const string& idCafeteriaConectada);

    void cargarMatrizInventario();
    bool actualizarMatrizProducto(const string& idProducto, int stock);
    bool obtenerPosicionProducto(const string& idProducto, int& fila, int& columna) const;

    void marcarUsuarioEnLinea(const string& username);
    void marcarUsuarioFueraDeLinea(const string& username);
    string obtenerUsuariosEnLinea();

public:
    Servidor();

    bool iniciar();
};

#endif
