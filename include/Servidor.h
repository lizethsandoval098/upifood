#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sys/types.h>

#include "BaseDatos.h"

using namespace std;

// =====================================================================
//  SERVIDOR UPIIFOOD  -  resumen de las tecnicas de SO que usa
// =====================================================================
//  SOCKETS   : TCP (socket/bind/listen/accept). Un hilo por conexion.
//  THREADS   : std::thread por cliente + un hilo dedicado a las senales.
//  MUTEX     : dbMutex, mutexInventario, mutexEventos, mutexLog, ... (ver abajo)
//  MATRICES  : inventario[2][20], ventas[2][20], pedidosPorEstado[2][5]
//  FORK      : al arrancar se hace fork() de un proceso "logger" (bitacora).
//  PIPE      : pipe #1 servidor -> logger (lineas de bitacora)
//              pipe #2 logger -> servidor (acuses de recibo)
//  SENALES   : SIGINT/SIGTERM apagado ordenado, SIGUSR1 estadisticas + flush
//              del logger, SIGUSR2 acuse del logger, SIGHUP recargar
//              inventario, SIGALRM latido periodico, SIGCHLD reaper,
//              SIGPIPE ignorado.
//
//  ORDEN GLOBAL DE CANDADOS (evita DEADLOCKS)
//  ------------------------------------------
//  Un deadlock necesita que dos hilos tomen los mismos candados en orden
//  distinto (A->B contra B->A). Aqui hay UN orden fijo y siempre se respeta:
//
//      1) dbMutex            (SQLite: lento, una sola conexion)
//      2) mutexInventario    (matrices en memoria: rapido)
//      3) mutexEventos       ) candados "hoja": nunca se pide otro candado
//      4) mutexLog           ) mientras se tienen, y se toman uno a la vez
//      5) mutexUsuariosEnLinea, mutexClientes, mutexConsola  (tambien hoja)
//
//  Cuando una operacion necesita 1 y 2 juntos se usa std::scoped_lock(dbMutex,
//  mutexInventario), que adquiere ambos con un algoritmo que NO puede
//  interbloquearse aunque otro codigo los pidiera al reves.
//
//  CONDICIONES DE CARRERA
//  ----------------------
//  "Revisar stock" y "descontar stock" ocurren dentro de la MISMA seccion
//  critica (check-then-act atomico). Los contadores compartidos son
//  std::atomic. Los datos compartidos (colas, listas, matrices) solo se
//  tocan con su mutex tomado.
// =====================================================================

class Servidor {

private:
    int socketServidor;
    int puerto;
    bool sembrarAlIniciar;

    // ---- Estado global ------------------------------------------------
    atomic<bool> apagando;
    atomic<int> hilosActivos;
    atomic<int> contadorFolio;
    atomic<long> comandosAtendidos;

    // ---- (1) Base de datos: UNA conexion SQLite compartida -------------
    BaseDatos db;
    mutex dbMutex;

    // ---- (2) Matrices de inventario ------------------------------------
    // Fila 0 -> cafeteria 1 (C1-01, C1-02, ...), fila 1 -> cafeteria 2 (C2-01, ...)
    // Columna 0 -> producto 01, columna 1 -> producto 02, etc.
    static const int MAX_PRODUCTOS_CAFETERIA = 20;
    static const int NUM_CAFETERIAS = 2;
    static const int NUM_ESTADOS = 5;   // Pendiente, Preparando, Listo, Entregado, Cancelado
    int inventario[2][MAX_PRODUCTOS_CAFETERIA];          // existencias
    int ventas[2][MAX_PRODUCTOS_CAFETERIA];              // unidades vendidas (netas)
    int pedidosPorEstado[2][NUM_ESTADOS];                // conteo de pedidos por estado
    mutex mutexInventario;

    // ---- (3) Bitacora de eventos en memoria (para el monitor en vivo) --
    struct Evento {
        long id;
        string hora;
        string tipo;
        string idCafeteria;
        string detalle;
    };
    deque<Evento> eventos;
    long siguienteIdEvento;
    mutex mutexEventos;

    // ---- (4) Proceso logger (fork) y sus pipes --------------------------
    pid_t pidLogger;
    int pipeLogEscritura;    // servidor -> logger
    int pipeAckLectura;      // logger -> servidor
    atomic<bool> loggerVivo;
    mutex mutexLog;

    // ---- (5) Otros candados hoja ----------------------------------------
    map<string, int> usuariosEnLinea;      // username -> numero de conexiones abiertas
    mutex mutexUsuariosEnLinea;
    set<int> socketsClientes;
    mutex mutexClientes;
    condition_variable cvClientes;
    mutex mutexConsola;

    // ---- Arranque / apagado -------------------------------------------
    bool iniciarLogger();                  // fork() + pipes
    static void procesoLogger(int fdLeer, int fdAck);
    void detenerLogger();
    void hiloSenales();                    // sigwait(): atiende SIGINT, SIGTERM, ...
    void apagar();
    void sembrarDatosDemo();

    // ---- Atencion de clientes -----------------------------------------
    void atenderCliente(int socketCliente);
    string procesarComando(const string& comando);

    // ---- Matrices --------------------------------------------------------
    void cargarMatrizInventario();
    bool actualizarMatrizProducto(const string& idProducto, int stock);
    bool obtenerPosicionProducto(const string& idProducto, int& fila, int& columna) const;
    void imprimirMatrices();
    string resumenEstadisticas();
    static int indiceEstado(const string& estado);

    // ---- Pedidos ---------------------------------------------------------
    string comandoCrearPedido(const vector<string>& campos);
    string comandoPagar(const vector<string>& campos);
    string comandoCambiarEstado(const vector<string>& campos);
    string comandoVentaCaja(const vector<string>& campos);
    string comandoRestock(const vector<string>& campos);
    string comandoEstadoPedido(const vector<string>& campos);
    string comandoListarPedidos(const vector<string>& campos);
    string comandoHistorial(const vector<string>& campos);
    string comandoTarjetas(const vector<string>& campos);
    string comandoEventos(const vector<string>& campos);
    void devolverStockPedido(const Pedido& pedido);   // requiere dbMutex+mutexInventario tomados
    static bool tarjetaValida(const string& numero);  // algoritmo de Luhn

    // ---- Usuarios en linea / eventos / bitacora -----------------------------
    void marcarUsuarioEnLinea(const string& username);
    void marcarUsuarioFueraDeLinea(const string& username);
    string obtenerUsuariosEnLinea();

    void registrarEvento(const string& tipo, const string& idCafeteria, const string& detalle);
    void escribirEnPipeLog(const string& linea);
    void imprimirConsola(const string& texto);

public:
    Servidor();
    ~Servidor();

    void configurar(int puerto, bool sembrar);
    bool iniciar();
};

#endif
