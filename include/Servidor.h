#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <random>

#include "BaseDatos.h"
#include "Tarjeta.h"

using namespace std;

class Servidor {

private:
    atomic<int> socketServidor;
    int puerto;

    // Una sola conexion a SQLite, compartida por los hilos del servidor.
    BaseDatos db;
    mutex dbMutex;

    // Usuarios conectados actualmente al servidor.
    vector<string> usuariosEnLinea;
    mutex mutexUsuariosEnLinea;

    // Matriz de inventario:
    // fila 0 -> cafeteria 1 (C1-01, C1-02, ...)
    // fila 1 -> cafeteria 2 (C2-01, C2-02, ...)  ... hasta la cafeteria 9
    // columna 0 -> producto 01, columna 1 -> producto 02, etc.
    static const int MAX_PRODUCTOS_CAFETERIA = 99; // IDs C1-01 ... C1-99
    static const int MAX_CAFETERIAS = 9;
    int inventario[MAX_CAFETERIAS][MAX_PRODUCTOS_CAFETERIA];
    mutex mutexInventario;

    // Contador de folios de pedidos. Solo se toca dentro de dbMutex
    // (misma seccion critica donde se genera e inserta el pedido), por
    // eso no necesita su propio mutex.
    long contadorPedidos;

    // ---- Control de apagado seguro (SIGINT/SIGTERM) ----
    atomic<bool> corriendo;

    // ---- Simulador de clientes (hilos + sockets hacia el propio servidor) ----
    atomic<bool> simuladorActivo;
    vector<thread> hilosSimulador;

    void atenderCliente(int socketCliente);
    string procesarComando(const string& comando);

    void cargarMatrizInventario();
    bool actualizarMatrizProducto(const string& idProducto, int stock);
    bool obtenerPosicionProducto(const string& idProducto, int& fila, int& columna) const;

    void marcarUsuarioEnLinea(const string& username);
    void marcarUsuarioFueraDeLinea(const string& username);
    string obtenerUsuariosEnLinea();

    // ---- Simulador de clientes ----
    void asegurarUsuariosSimulados();
    // Regresa la tarjeta simulada del bot; si todavia no tiene, la genera y la
    // guarda en la BD. El que llama YA debe tener tomado dbMutex.
    Tarjeta tarjetaBotSinBloqueo(const string& username);
    void iniciarSimuladorClientes();
    void hiloSimuladorClientes(int idHilo);
    bool crearPedidoSimulado(int idHilo, mt19937& generador);

public:
    Servidor();
    ~Servidor();

    bool iniciar();

    // Apagado seguro: deja de aceptar conexiones, detiene los hilos
    // simuladores y libera la base de datos. Pensado para llamarse desde
    // un manejador de SIGINT/SIGTERM (indirectamente, via un hilo monitor).
    void detener();

    // Enciende/apaga el simulador de clientes en caliente (pensado para
    // invocarse al recibir SIGUSR2).
    void alternarSimulador();

    // Genera un reporte de ventas usando un proceso hijo (fork + pipe)
    // para aislar el trabajo pesado de formateo/escritura a disco.
    // Pensado para invocarse al recibir SIGUSR1.
    bool generarReporteVentas();
};

#endif
