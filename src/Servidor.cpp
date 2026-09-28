#include "Servidor.h"
#include "Protocolo.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Usuario.h"
#include "Producto.h"
#include "Pedido.h"
#include "Pago.h"
#include "Tarjeta.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

static const int MAX_CONEXIONES = 128;
static const int MAX_EVENTOS_EN_MEMORIA = 1000;
static const int SEGUNDOS_LATIDO = 30;

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

static string horaActual(bool conFecha = false) {
    time_t t = time(nullptr);
    tm resultado;
    localtime_r(&t, &resultado);   // localtime_r: version thread-safe

    char buffer[32];
    strftime(buffer, sizeof(buffer), conFecha ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S", &resultado);
    return string(buffer);
}

static string sanear(const string& texto) {
    string r = texto;
    for (char& c : r) {
        if (c == '|' || c == '\n' || c == '\r') c = '/';
    }
    return r;
}

// =====================================================================
//  Proceso LOGGER (hijo creado con fork)
//  - Lee lineas del pipe #1 y las agrega a logs/servidor.log
//  - SIGUSR1 (del padre): hace flush y responde por el pipe #2 + SIGUSR2
//  - SIGTERM o EOF del pipe: termina despues de vaciar el pipe
//  - Ignora SIGINT: si el usuario hace Ctrl+C, el padre coordina el cierre
//    y el logger sigue vivo hasta guardar las ultimas lineas.
// =====================================================================

static volatile sig_atomic_t g_loggerFlush = 0;
static volatile sig_atomic_t g_loggerTerm = 0;

static void loggerManejadorUsr1(int) { g_loggerFlush = 1; }
static void loggerManejadorTerm(int) { g_loggerTerm = 1; }

void Servidor::procesoLogger(int fdLeer, int fdAck) {
    pid_t padre = getppid();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = loggerManejadorUsr1;
    sigaction(SIGUSR1, &sa, nullptr);

    sa.sa_handler = loggerManejadorTerm;
    sigaction(SIGTERM, &sa, nullptr);

    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    mkdir("logs", 0755);
    FILE* archivo = fopen("logs/servidor.log", "a");
    if (archivo == nullptr) {
        archivo = stderr;
    }

    long lineas = 0;
    string acumulado;
    char buffer[1024];

    fprintf(archivo, "=== Logger iniciado (pid %d, padre %d) %s ===\n",
            getpid(), padre, horaActual(true).c_str());
    fflush(archivo);

    auto volcar = [&](const char* datos, ssize_t n) {
        acumulado.append(datos, static_cast<size_t>(n));
        size_t pos;
        while ((pos = acumulado.find('\n')) != string::npos) {
            fprintf(archivo, "%s\n", acumulado.substr(0, pos).c_str());
            acumulado.erase(0, pos + 1);
            lineas++;
        }
    };

    while (true) {
        pollfd pfd;
        pfd.fd = fdLeer;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int r = poll(&pfd, 1, 500);   // 500 ms: asi revisa las banderas seguido

        if (r > 0) {
            ssize_t n = read(fdLeer, buffer, sizeof(buffer));

            if (n > 0) {
                volcar(buffer, n);
                fflush(archivo);
            } else if (n == 0) {
                break;                // EOF: el servidor cerro su lado del pipe
            } else if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        } else if (r < 0 && errno != EINTR) {
            break;
        }

        if (g_loggerFlush) {
            g_loggerFlush = 0;
            fflush(archivo);
            fsync(fileno(archivo));

            char mensaje[128];
            snprintf(mensaje, sizeof(mensaje), "ACK|lineas=%ld|pid=%d\n", lineas, getpid());
            ssize_t w = write(fdAck, mensaje, strlen(mensaje));
            (void)w;

            kill(padre, SIGUSR2);     // "intercambio de senales": le avisamos al padre
        }

        if (g_loggerTerm) {
            // vaciar lo que quede en el pipe sin bloquear
            int flags = fcntl(fdLeer, F_GETFL, 0);
            fcntl(fdLeer, F_SETFL, flags | O_NONBLOCK);
            ssize_t n;
            while ((n = read(fdLeer, buffer, sizeof(buffer))) > 0) {
                volcar(buffer, n);
            }
            break;
        }

        if (getppid() != padre) {
            break;                    // el servidor murio: no quedarse huerfano
        }
    }

    fprintf(archivo, "=== Logger termina (%ld lineas) %s ===\n", lineas, horaActual(true).c_str());
    fflush(archivo);

    if (archivo != stderr) fclose(archivo);

    close(fdLeer);
    close(fdAck);
}

// ---------------------------------------------------------------------
// Constructor / destructor / configuracion
// ---------------------------------------------------------------------

Servidor::Servidor() {
    socketServidor = -1;
    puerto = obtenerPuertoServidor();
    sembrarAlIniciar = false;

    apagando = false;
    hilosActivos = 0;
    contadorFolio = 0;
    comandosAtendidos = 0;

    siguienteIdEvento = 0;

    pidLogger = -1;
    pipeLogEscritura = -1;
    pipeAckLectura = -1;
    loggerVivo = false;

    for (int i = 0; i < NUM_CAFETERIAS; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            inventario[i][j] = 0;
            ventas[i][j] = 0;
        }
        for (int k = 0; k < NUM_ESTADOS; k++) {
            pedidosPorEstado[i][k] = 0;
        }
    }
}

Servidor::~Servidor() {
    if (pipeLogEscritura != -1) close(pipeLogEscritura);
    if (pipeAckLectura != -1) close(pipeAckLectura);
    if (socketServidor != -1) close(socketServidor);
}

void Servidor::configurar(int nuevoPuerto, bool sembrar) {
    if (nuevoPuerto > 0 && nuevoPuerto < 65536) puerto = nuevoPuerto;
    sembrarAlIniciar = sembrar;
}

// ---------------------------------------------------------------------
// fork() + pipes: proceso logger
// ---------------------------------------------------------------------

bool Servidor::iniciarLogger() {
    int aLogger[2];    // servidor -> logger
    int deLogger[2];   // logger -> servidor

    if (pipe(aLogger) == -1) {
        perror("pipe");
        return false;
    }

    if (pipe(deLogger) == -1) {
        perror("pipe");
        close(aLogger[0]);
        close(aLogger[1]);
        return false;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        close(aLogger[0]); close(aLogger[1]);
        close(deLogger[0]); close(deLogger[1]);
        return false;
    }

    if (pid == 0) {
        // ---------------- HIJO ----------------
        close(aLogger[1]);     // el hijo solo lee del pipe #1
        close(deLogger[0]);    // y solo escribe en el pipe #2
        procesoLogger(aLogger[0], deLogger[1]);
        _exit(0);              // _exit: no ejecutar destructores heredados del padre
    }

    // ---------------- PADRE ----------------
    close(aLogger[0]);
    close(deLogger[1]);

    pipeLogEscritura = aLogger[1];
    pipeAckLectura = deLogger[0];
    pidLogger = pid;
    loggerVivo = true;

    // Escritura sin bloqueo: si el logger se atrasa, se pierde una linea de
    // bitacora pero jamas se congela un hilo de atencion.
    fcntl(pipeLogEscritura, F_SETFL, fcntl(pipeLogEscritura, F_GETFL, 0) | O_NONBLOCK);
    fcntl(pipeAckLectura, F_SETFL, fcntl(pipeAckLectura, F_GETFL, 0) | O_NONBLOCK);

    return true;
}

void Servidor::detenerLogger() {
    if (pidLogger <= 0) return;

    {
        lock_guard<mutex> guard(mutexLog);
        if (pipeLogEscritura != -1) {
            close(pipeLogEscritura);   // EOF para el logger: vacia y termina solo
            pipeLogEscritura = -1;
        }
    }

    // Se le da tiempo de terminar solo; si no, SIGTERM y al final SIGKILL.
    for (int intento = 0; intento < 60; intento++) {
        int estado = 0;
        pid_t r = waitpid(pidLogger, &estado, WNOHANG);

        if (r == pidLogger || (r == -1 && errno == ECHILD)) {
            loggerVivo = false;
            pidLogger = -1;
            return;
        }

        if (intento == 20) kill(pidLogger, SIGTERM);
        if (intento == 50) kill(pidLogger, SIGKILL);

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    waitpid(pidLogger, nullptr, 0);
    loggerVivo = false;
    pidLogger = -1;
}

void Servidor::escribirEnPipeLog(const string& linea) {
    if (!loggerVivo) return;

    lock_guard<mutex> guard(mutexLog);

    if (pipeLogEscritura == -1) return;

    string dato = linea.substr(0, 900) + "\n";     // < PIPE_BUF: escritura atomica
    ssize_t n = write(pipeLogEscritura, dato.c_str(), dato.size());

    if (n < 0 && errno == EPIPE) {
        loggerVivo = false;                        // el logger murio
    }
    // EAGAIN (pipe lleno): se descarta la linea a proposito.
}

void Servidor::imprimirConsola(const string& texto) {
    lock_guard<mutex> guard(mutexConsola);
    cout << texto << endl;
}

// ---------------------------------------------------------------------
// Bitacora de eventos
// ---------------------------------------------------------------------

void Servidor::registrarEvento(const string& tipo, const string& idCafeteria, const string& detalle) {
    Evento e;
    e.tipo = tipo;
    e.idCafeteria = idCafeteria;
    e.detalle = sanear(detalle);
    e.hora = horaActual();

    {
        lock_guard<mutex> guard(mutexEventos);
        e.id = ++siguienteIdEvento;
        eventos.push_back(e);

        while (static_cast<int>(eventos.size()) > MAX_EVENTOS_EN_MEMORIA) {
            eventos.pop_front();
        }
    }

    // Los candados de salida se toman DESPUES y uno a la vez (nunca anidados).
    string linea = "[" + e.hora + "] #" + to_string(e.id) + " " + tipo +
                   (idCafeteria.empty() ? "" : " caf=" + idCafeteria) + " " + e.detalle;

    imprimirConsola(linea);
    escribirEnPipeLog(linea);
}

// ---------------------------------------------------------------------
// Arranque
// ---------------------------------------------------------------------

bool Servidor::iniciar() {
    // 1) fork() ANTES de crear hilos ni abrir la BD: hacer fork() en un proceso
    //    con varios hilos es peligroso (el hijo solo copia el hilo que llama).
    if (!iniciarLogger()) {
        cout << "No se pudo crear el proceso logger (fork/pipe)." << endl;
        return false;
    }

    // 2) Senales: se BLOQUEAN en el hilo principal; todos los hilos que se creen
    //    despues heredan la mascara. Un unico hilo (hiloSenales) las recibe con
    //    sigwait(), asi el handler puede usar mutex y cout con seguridad.
    sigset_t conjunto;
    sigemptyset(&conjunto);
    sigaddset(&conjunto, SIGINT);
    sigaddset(&conjunto, SIGTERM);
    sigaddset(&conjunto, SIGUSR1);
    sigaddset(&conjunto, SIGUSR2);
    sigaddset(&conjunto, SIGHUP);
    sigaddset(&conjunto, SIGALRM);
    sigaddset(&conjunto, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &conjunto, nullptr);

    signal(SIGPIPE, SIG_IGN);

    // 3) Base de datos + matrices
    if (!db.conectar()) {
        cout << "No se pudo abrir la base de datos. El servidor no puede iniciar." << endl;
        detenerLogger();
        return false;
    }

    bool baseVacia;
    {
        lock_guard<mutex> guard(dbMutex);
        contadorFolio = db.contarPedidosTotales();
        baseVacia = db.obtenerCafeterias().empty();
    }

    if (sembrarAlIniciar || baseVacia) {
        sembrarDatosDemo();
    }

    cargarMatrizInventario();

    // 4) Socket TCP
    socketServidor = socket(AF_INET, SOCK_STREAM, 0);

    if (socketServidor == -1) {
        cout << "Error creando el socket del servidor." << endl;
        detenerLogger();
        return false;
    }

    int opt = 1;
    setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in direccion;
    memset(&direccion, 0, sizeof(direccion));
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(puerto);

    if (bind(socketServidor, (sockaddr*)&direccion, sizeof(direccion)) == -1) {
        cout << "Error haciendo bind en el puerto " << puerto << "." << endl;
        detenerLogger();
        return false;
    }

    if (listen(socketServidor, 64) == -1) {
        cout << "Error poniendo el socket en modo escucha." << endl;
        detenerLogger();
        return false;
    }

    // 5) Hilo de senales
    thread hiloSig(&Servidor::hiloSenales, this);

    registrarEvento("SERVIDOR_INICIO", "",
                    "puerto=" + to_string(puerto) + " pid=" + to_string(getpid()) +
                    " logger_pid=" + to_string(pidLogger));

    imprimirConsola("Servidor escuchando en el puerto " + to_string(puerto) + " (pid " + to_string(getpid()) + ")");
    imprimirConsola("  kill -USR1 " + to_string(getpid()) + "   -> estadisticas + matrices + flush del logger");
    imprimirConsola("  kill -HUP  " + to_string(getpid()) + "   -> recargar inventario desde la BD");
    imprimirConsola("  kill -TERM " + to_string(getpid()) + "   -> apagado ordenado (o Ctrl+C)");

    // 6) Ciclo de aceptacion: un hilo por conexion
    while (!apagando) {
        sockaddr_in direccionCliente;
        socklen_t tam = sizeof(direccionCliente);

        int socketCliente = accept(socketServidor, (sockaddr*)&direccionCliente, &tam);

        if (socketCliente == -1) {
            if (apagando) break;
            if (errno == EINTR || errno == ECONNABORTED) continue;
            perror("accept");
            break;
        }

        if (hilosActivos >= MAX_CONEXIONES) {
            enviarMensaje(socketCliente, "ERR|Servidor lleno, intenta mas tarde.");
            close(socketCliente);
            continue;
        }

        {
            // El contador y el conjunto de sockets se actualizan bajo el mismo
            // candado que usa el cv del apagado (evita "wakeups perdidos").
            lock_guard<mutex> guard(mutexClientes);
            socketsClientes.insert(socketCliente);
            hilosActivos++;
        }

        try {
            thread hiloCliente(&Servidor::atenderCliente, this, socketCliente);
            hiloCliente.detach();
        } catch (...) {
            lock_guard<mutex> guard(mutexClientes);
            socketsClientes.erase(socketCliente);
            hilosActivos--;
            close(socketCliente);
        }
    }

    // 7) Si el ciclo termino por un error (no por senal), se pide el apagado.
    if (!apagando) {
        kill(getpid(), SIGTERM);
    }

    hiloSig.join();

    // 8) Esperar a que terminen los hilos de atencion (maximo 5 s)
    {
        unique_lock<mutex> lk(mutexClientes);
        bool todosTerminaron = cvClientes.wait_for(lk, chrono::seconds(5),
                                                   [this] { return hilosActivos == 0; });
        if (!todosTerminaron) {
            imprimirConsola("Aviso: quedaron " + to_string(hilosActivos.load()) + " conexiones sin cerrar.");
        }
    }

    registrarEvento("SERVIDOR_FIN", "", "comandos_atendidos=" + to_string(comandosAtendidos.load()));

    {
        lock_guard<mutex> guard(dbMutex);
        db.desconectar();
    }

    close(socketServidor);
    socketServidor = -1;

    detenerLogger();
    imprimirConsola("Servidor detenido correctamente.");

    return true;
}

// ---------------------------------------------------------------------
// Hilo de senales (sigwait)
// ---------------------------------------------------------------------

void Servidor::apagar() {
    apagando = true;

    // shutdown() sobre el socket que escucha despierta al accept() bloqueado.
    if (socketServidor != -1) {
        shutdown(socketServidor, SHUT_RDWR);
    }

    // Y sobre cada cliente despierta su recv() bloqueado.
    lock_guard<mutex> guard(mutexClientes);
    for (int fd : socketsClientes) {
        shutdown(fd, SHUT_RDWR);
    }
}

void Servidor::hiloSenales() {
    sigset_t conjunto;
    sigemptyset(&conjunto);
    sigaddset(&conjunto, SIGINT);
    sigaddset(&conjunto, SIGTERM);
    sigaddset(&conjunto, SIGUSR1);
    sigaddset(&conjunto, SIGUSR2);
    sigaddset(&conjunto, SIGHUP);
    sigaddset(&conjunto, SIGALRM);
    sigaddset(&conjunto, SIGCHLD);

    alarm(SEGUNDOS_LATIDO);

    while (true) {
        int senal = 0;

        if (sigwait(&conjunto, &senal) != 0) {
            continue;
        }

        switch (senal) {
            case SIGINT:
            case SIGTERM: {
                if (apagando) {
                    imprimirConsola("Segunda senal de terminacion: salida inmediata.");
                    _exit(1);
                }

                registrarEvento("SENAL", "", string("recibida ") + (senal == SIGINT ? "SIGINT" : "SIGTERM") +
                                             " -> apagado ordenado");
                alarm(0);
                apagar();
                return;
            }

            case SIGUSR1: {
                registrarEvento("SENAL", "", "recibida SIGUSR1 -> estadisticas");
                imprimirMatrices();

                if (loggerVivo && pidLogger > 0) {
                    kill(pidLogger, SIGUSR1);   // pedirle al logger que haga flush
                }
                break;
            }

            case SIGUSR2: {
                // Acuse del logger (ver procesoLogger): se lee del pipe #2.
                char buffer[256];
                ssize_t n = read(pipeAckLectura, buffer, sizeof(buffer) - 1);

                if (n > 0) {
                    buffer[n] = '\0';
                    string ack(buffer);
                    while (!ack.empty() && (ack.back() == '\n' || ack.back() == '\r')) ack.pop_back();
                    imprimirConsola("[senales] SIGUSR2 del logger, pipe dice: " + ack);
                } else {
                    imprimirConsola("[senales] SIGUSR2 recibida (sin datos en el pipe).");
                }
                break;
            }

            case SIGHUP: {
                registrarEvento("SENAL", "", "recibida SIGHUP -> recargando inventario desde la BD");
                cargarMatrizInventario();
                break;
            }

            case SIGALRM: {
                int enLinea;
                {
                    lock_guard<mutex> guard(mutexUsuariosEnLinea);
                    enLinea = static_cast<int>(usuariosEnLinea.size());
                }

                imprimirConsola("[latido] conexiones=" + to_string(hilosActivos.load()) +
                                " usuarios_en_linea=" + to_string(enLinea) +
                                " comandos=" + to_string(comandosAtendidos.load()) +
                                " logger=" + (loggerVivo ? "vivo" : "caido"));
                alarm(SEGUNDOS_LATIDO);
                break;
            }

            case SIGCHLD: {
                // Reaper: recoge al logger si termino (evita procesos zombie).
                if (pidLogger > 0) {
                    int estado = 0;
                    pid_t r = waitpid(pidLogger, &estado, WNOHANG);

                    if (r == pidLogger) {
                        loggerVivo = false;
                        pidLogger = -1;
                        imprimirConsola("[senales] SIGCHLD: el proceso logger termino (estado " +
                                        to_string(WEXITSTATUS(estado)) + ").");
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------
// Atencion de un cliente (un hilo por conexion)
// ---------------------------------------------------------------------

void Servidor::atenderCliente(int socketCliente) {
    string usuarioConectado;

    while (!apagando) {
        string comando;

        if (!recibirMensaje(socketCliente, comando)) {
            break;
        }

        comandosAtendidos++;

        vector<string> camposComando = separarCampos(comando);

        if (camposComando.empty()) {
            if (!enviarMensaje(socketCliente, "ERR|Comando vacio.")) break;
            continue;
        }

        // No se imprimen contrasenas ni tarjetas en la consola/bitacora.
        const string& tipo = camposComando[0];
        bool sensible = (tipo.rfind("LOGIN_", 0) == 0 || tipo == "REGISTRO_CLIENTE" || tipo == "PAGAR");

        if (tipo != "EVENTOS") {   // el monitor consulta cada segundo; no ensuciar la consola
            imprimirConsola("Comando recibido: " + (sensible ? tipo + "|" + (camposComando.size() > 1 ? camposComando[1] : "") + "|***" : comando));
        }

        string respuesta = procesarComando(comando);
        vector<string> camposRespuesta = separarCampos(respuesta);

        if (!camposRespuesta.empty() && camposRespuesta[0] == "OK") {
            string usuario, rol, idCaf;

            if (tipo == "LOGIN_CLIENTE" && camposRespuesta.size() >= 7) {
                usuario = camposRespuesta[6];
                rol = "Cliente";
            }
            else if (tipo == "LOGIN_CAFETERIA" && camposRespuesta.size() >= 5) {
                usuario = camposRespuesta[4];
                rol = "Cafe";
                idCaf = camposRespuesta[3];
            }
            else if (tipo == "LOGIN_ADMIN" && camposRespuesta.size() >= 4) {
                usuario = camposRespuesta[3];
                rol = "Admin";
            }

            if (!usuario.empty()) {
                if (!usuarioConectado.empty()) {
                    marcarUsuarioFueraDeLinea(usuarioConectado);   // re-login en la misma conexion
                }
                usuarioConectado = usuario;
                marcarUsuarioEnLinea(usuarioConectado);
                registrarEvento("LOGIN", idCaf, "usuario=" + usuario + " rol=" + rol);
            }
        }

        if (!enviarMensaje(socketCliente, respuesta)) {
            break;
        }
    }

    if (!usuarioConectado.empty()) {
        marcarUsuarioFueraDeLinea(usuarioConectado);
        registrarEvento("DESCONEXION", "", "usuario=" + usuarioConectado);
    }

    close(socketCliente);

    {
        lock_guard<mutex> guard(mutexClientes);
        socketsClientes.erase(socketCliente);
        hilosActivos--;
    }
    cvClientes.notify_all();
}

// ---------------------------------------------------------------------
// Matrices de inventario / ventas / estados
// ---------------------------------------------------------------------

int Servidor::indiceEstado(const string& estado) {
    static const char* nombres[NUM_ESTADOS] = {"Pendiente", "Preparando", "Listo", "Entregado", "Cancelado"};

    for (int i = 0; i < NUM_ESTADOS; i++) {
        if (estado == nombres[i]) return i;
    }

    return -1;
}

// Reconstruye las tres matrices a partir de la BD (arranque y SIGHUP).
void Servidor::cargarMatrizInventario() {
    // scoped_lock toma AMBOS candados con un algoritmo anti-deadlock.
    scoped_lock lk(dbMutex, mutexInventario);

    for (int i = 0; i < NUM_CAFETERIAS; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            inventario[i][j] = 0;
            ventas[i][j] = 0;
        }
        for (int k = 0; k < NUM_ESTADOS; k++) {
            pedidosPorEstado[i][k] = 0;
        }
    }

    for (int cafeteria = 1; cafeteria <= NUM_CAFETERIAS; cafeteria++) {
        vector<Producto> productos = db.obtenerInventario(to_string(cafeteria));

        for (const auto& producto : productos) {
            int fila = 0;
            int columna = 0;

            if (obtenerPosicionProducto(producto.getIdProducto(), fila, columna)) {
                inventario[fila][columna] = producto.getStock();
            }
        }

        // ventas y conteo por estado salen de los pedidos ya guardados
        vector<Pedido> pedidos = db.obtenerPedidosCafeteria(to_string(cafeteria));

        for (const auto& pedido : pedidos) {
            int e = indiceEstado(pedido.getEstado());
            if (e >= 0) pedidosPorEstado[cafeteria - 1][e]++;

            if (pedido.getEstado() == "Cancelado") continue;

            for (const auto& item : pedido.getListaProductos()) {
                int fila = 0;
                int columna = 0;

                if (obtenerPosicionProducto(item.first.getIdProducto(), fila, columna)) {
                    ventas[fila][columna] += item.second;
                }
            }
        }
    }
}

bool Servidor::obtenerPosicionProducto(const string& idProducto, int& fila, int& columna) const {
    // Formato esperado: C1-01, C1-02, ..., C2-01, C2-02, ...
    if (idProducto.size() != 5) {
        return false;
    }

    if (idProducto[0] != 'C' || idProducto[2] != '-') {
        return false;
    }

    if (idProducto[1] != '1' && idProducto[1] != '2') {
        return false;
    }

    if (idProducto[3] < '0' || idProducto[3] > '9' ||
        idProducto[4] < '0' || idProducto[4] > '9') {
        return false;
    }

    int numeroProducto = (idProducto[3] - '0') * 10 + (idProducto[4] - '0');

    if (numeroProducto < 1 || numeroProducto > MAX_PRODUCTOS_CAFETERIA) {
        return false;
    }

    fila = (idProducto[1] == '1') ? 0 : 1;
    columna = numeroProducto - 1;
    return true;
}

bool Servidor::actualizarMatrizProducto(const string& idProducto, int stock) {
    int fila = 0;
    int columna = 0;

    if (!obtenerPosicionProducto(idProducto, fila, columna)) {
        return false;
    }

    inventario[fila][columna] = stock;   // el llamador ya tiene mutexInventario
    return true;
}

void Servidor::imprimirMatrices() {
    int inv[NUM_CAFETERIAS][MAX_PRODUCTOS_CAFETERIA];
    int ven[NUM_CAFETERIAS][MAX_PRODUCTOS_CAFETERIA];
    int est[NUM_CAFETERIAS][NUM_ESTADOS];

    {
        // Se copia rapido y se suelta el candado ANTES de imprimir (no se
        // hace I/O lenta con un candado tomado).
        lock_guard<mutex> guard(mutexInventario);
        memcpy(inv, inventario, sizeof(inv));
        memcpy(ven, ventas, sizeof(ven));
        memcpy(est, pedidosPorEstado, sizeof(est));
    }

    int columnas = 0;
    for (int i = 0; i < NUM_CAFETERIAS; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            if (inv[i][j] != 0 || ven[i][j] != 0) columnas = max(columnas, j + 1);
        }
    }
    if (columnas == 0) columnas = 8;

    ostringstream out;

    auto imprimirMatriz = [&](const string& titulo, int m[NUM_CAFETERIAS][MAX_PRODUCTOS_CAFETERIA]) {
        out << "\n=== " << titulo << " ===\n";
        out << "      ";
        for (int j = 0; j < columnas; j++) out << setw(5) << (j + 1);
        out << "  | suma fila\n";

        int sumaColumna[MAX_PRODUCTOS_CAFETERIA] = {0};

        for (int i = 0; i < NUM_CAFETERIAS; i++) {
            out << "  C" << (i + 1) << "  ";
            int sumaFila = 0;

            for (int j = 0; j < columnas; j++) {
                out << setw(5) << m[i][j];
                sumaFila += m[i][j];
                sumaColumna[j] += m[i][j];
            }

            out << "  | " << sumaFila << "\n";
        }

        out << "  Σcol";
        for (int j = 0; j < columnas; j++) out << setw(5) << sumaColumna[j];
        out << "\n";
    };

    imprimirMatriz("MATRIZ inventario[cafeteria][producto] (existencias)", inv);
    imprimirMatriz("MATRIZ ventas[cafeteria][producto] (unidades netas vendidas)", ven);

    out << "\n=== MATRIZ pedidosPorEstado[cafeteria][estado] ===\n";
    out << "        Pend Prep Listo Entr Canc\n";
    for (int i = 0; i < NUM_CAFETERIAS; i++) {
        out << "  C" << (i + 1) << "  ";
        for (int k = 0; k < NUM_ESTADOS; k++) out << setw(5) << est[i][k];
        out << "\n";
    }

    imprimirConsola(out.str());
}

string Servidor::resumenEstadisticas() {
    lock_guard<mutex> guard(mutexInventario);

    string respuesta = "OK|" + to_string(NUM_CAFETERIAS);

    for (int i = 0; i < NUM_CAFETERIAS; i++) {
        long stockTotal = 0;
        long vendidas = 0;

        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            stockTotal += inventario[i][j];
            vendidas += ventas[i][j];
        }

        respuesta += "\n" + to_string(i + 1) + "|" + to_string(stockTotal) + "|" + to_string(vendidas);

        for (int k = 0; k < NUM_ESTADOS; k++) {
            respuesta += "|" + to_string(pedidosPorEstado[i][k]);
        }
    }

    return respuesta;
}

// ---------------------------------------------------------------------
// Usuarios en linea
// ---------------------------------------------------------------------

void Servidor::marcarUsuarioEnLinea(const string& username) {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);
    usuariosEnLinea[username]++;
}

void Servidor::marcarUsuarioFueraDeLinea(const string& username) {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    auto it = usuariosEnLinea.find(username);

    if (it != usuariosEnLinea.end() && --(it->second) <= 0) {
        usuariosEnLinea.erase(it);
    }
}

string Servidor::obtenerUsuariosEnLinea() {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    string respuesta = "OK|" + to_string(usuariosEnLinea.size());

    for (const auto& par : usuariosEnLinea) {
        respuesta += "\n" + par.first;
    }

    return respuesta;
}

// ---------------------------------------------------------------------
// Datos de demostracion (solo si la BD esta vacia o con --seed)
// ---------------------------------------------------------------------

void Servidor::sembrarDatosDemo() {
    lock_guard<mutex> guard(dbMutex);

    Usuario admin("Admin", "Administrador UPIIFOOD", "admin@upiifood.mx", "admin1234", "admin");
    db.guardarAdministrador(admin);

    Cafeteria c1("Cafeteria UPIITA 1", "cafe1@upiifood.mx", "cafe1234", "cafe1", "1");
    Cafeteria c2("Cafeteria UPIITA 2", "cafe2@upiifood.mx", "cafe1234", "cafe2", "2");
    db.guardarCafeteria(c1);
    db.guardarCafeteria(c2);

    struct Item { const char* id; const char* nombre; int stock; float precio; };

    const Item catalogo[] = {
        {"C1-01", "Cafe americano", 30, 25.0f}, {"C1-02", "Capuchino", 30, 35.0f},
        {"C1-03", "Latte", 30, 38.0f},          {"C1-04", "Te chai", 30, 30.0f},
        {"C1-05", "Sandwich", 25, 45.0f},       {"C1-06", "Baguette de jamon", 25, 50.0f},
        {"C1-07", "Galleta", 40, 15.0f},        {"C1-08", "Pan dulce", 40, 18.0f},
        {"C2-01", "Torta", 25, 55.0f},          {"C2-02", "Molletes", 25, 40.0f},
        {"C2-03", "Chilaquiles", 20, 60.0f},    {"C2-04", "Jugo de naranja", 30, 25.0f},
        {"C2-05", "Agua natural", 40, 15.0f},   {"C2-06", "Refresco", 40, 22.0f},
        {"C2-07", "Quesadilla", 30, 35.0f},     {"C2-08", "Ensalada", 20, 50.0f},
    };

    for (const auto& item : catalogo) {
        db.guardarProducto(Producto(item.nombre, item.id, item.stock, item.precio));
    }

    cout << "Datos de demostracion sembrados (los que ya existian se conservan):" << endl;
    cout << "  admin  / admin1234   (administrador)" << endl;
    cout << "  cafe1  / cafe1234    (cafeteria 1)" << endl;
    cout << "  cafe2  / cafe1234    (cafeteria 2)" << endl;
}

// ---------------------------------------------------------------------
// Utilidades de pedidos
// ---------------------------------------------------------------------

// Algoritmo de Luhn: es el mismo que valida numeros de tarjeta reales.
bool Servidor::tarjetaValida(const string& numero) {
    if (numero.size() < 13 || numero.size() > 19) return false;

    int suma = 0;
    bool duplicar = false;

    for (int i = static_cast<int>(numero.size()) - 1; i >= 0; i--) {
        if (numero[i] < '0' || numero[i] > '9') return false;

        int digito = numero[i] - '0';

        if (duplicar) {
            digito *= 2;
            if (digito > 9) digito -= 9;
        }

        suma += digito;
        duplicar = !duplicar;
    }

    return suma % 10 == 0;
}

// Requiere dbMutex y mutexInventario ya tomados por el llamador.
void Servidor::devolverStockPedido(const Pedido& pedido) {
    for (const auto& item : pedido.getListaProductos()) {
        int fila = 0;
        int columna = 0;

        if (!obtenerPosicionProducto(item.first.getIdProducto(), fila, columna)) continue;

        inventario[fila][columna] += item.second;
        ventas[fila][columna] -= item.second;
        db.actualizarExistencia(item.first.getIdProducto(), inventario[fila][columna]);
    }
}

// ---------------------------------------------------------------------
// CREAR_PEDIDO|username|idCafeteria|C1-01:2;C1-03:1
// ---------------------------------------------------------------------

string Servidor::comandoCrearPedido(const vector<string>& campos) {
    if (campos.size() < 4) return "ERR|Formato invalido. Uso: CREAR_PEDIDO|user|idCafeteria|id:cant;id:cant";

    const string& username = campos[1];
    const string& idCaf = campos[2];

    if (username.empty()) return "ERR|Username vacio.";
    if (idCaf != "1" && idCaf != "2") return "ERR|Cafeteria invalida.";

    // ---- parseo y validacion del carrito (sin candados: solo datos locales) ----
    map<string, int> pedidoItems;

    for (const string& parte : separarCampos(campos[3], ';')) {
        if (parte.empty()) continue;

        vector<string> par = separarCampos(parte, ':');
        if (par.size() != 2) return "ERR|Carrito mal formado.";

        int cantidad = 0;
        try {
            cantidad = stoi(par[1]);
        } catch (...) {
            return "ERR|Cantidad invalida.";
        }

        int f = 0, c = 0;
        if (!obtenerPosicionProducto(par[0], f, c) || par[0][1] != idCaf[0]) {
            return "ERR|Producto " + par[0] + " no pertenece a la cafeteria " + idCaf + ".";
        }

        if (cantidad < 1 || cantidad > 50) return "ERR|La cantidad debe estar entre 1 y 50.";

        pedidoItems[par[0]] += cantidad;
    }

    if (pedidoItems.empty()) return "ERR|El carrito esta vacio.";
    if (pedidoItems.size() > 15) return "ERR|Demasiados productos distintos (max 15).";

    const int fila = (idCaf == "1") ? 0 : 1;
    string folio;
    float total = 0.0f;
    string detalleItems;

    {
        // ===== SECCION CRITICA =====
        // Revisar el stock y descontarlo ocurre TODO con los dos candados
        // tomados. Si se revisara con un candado y se descontara despues con
        // otro, dos clientes podrian ver "queda 1" al mismo tiempo y comprar
        // los dos (condicion de carrera / TOCTOU).
        scoped_lock lk(dbMutex, mutexInventario);

        Cliente usuario = db.obtenerUsuarioCliente(username);

        if (usuario.getUsername().empty()) {
            if (username.rfind("inv_", 0) == 0) {
                // Cliente invitado: se registra automaticamente como INVITADO.
                Cliente invitado(username, "invitado@upiifood.local", "inv-" + username, username,
                                 "", "", "INVITADO");

                if (!db.guardarUsuarioCliente(invitado)) {
                    return "ERR|No se pudo registrar al invitado.";
                }
            } else {
                return "ERR|Usuario inexistente. Inicia sesion o entra como invitado.";
            }
        }

        vector<Producto> catalogo = db.obtenerInventario(idCaf);
        vector<pair<Producto, int>> lista;

        // (1) REVISAR todo el carrito primero: todo o nada.
        for (const auto& item : pedidoItems) {
            const Producto* encontrado = nullptr;

            for (const auto& p : catalogo) {
                if (p.getIdProducto() == item.first) { encontrado = &p; break; }
            }

            if (encontrado == nullptr) return "ERR|Producto inexistente: " + item.first;

            int f = 0, c = 0;
            obtenerPosicionProducto(item.first, f, c);

            if (inventario[f][c] < item.second) {
                return "ERR|Sin stock suficiente de " + encontrado->getNombreProducto() +
                       " (disponible: " + to_string(inventario[f][c]) + ")";
            }

            lista.push_back({*encontrado, item.second});
            total += encontrado->getPrecio() * item.second;
            detalleItems += (detalleItems.empty() ? "" : ",") + item.first + "x" + to_string(item.second);
        }

        // (2) GUARDAR el pedido (la BD es la fuente de verdad)
        Pedido pedido;
        pedido.setUsernameCliente(username);
        pedido.setIdCafeteria(idCaf);
        pedido.setListaProductos(lista);
        pedido.setTotal(total);
        pedido.setEstado("Pendiente");
        pedido.setFecha(horaActual(true));

        bool guardado = false;

        for (int intento = 0; intento < 5 && !guardado; intento++) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "P%s-%05d", idCaf.c_str(), ++contadorFolio);
            pedido.setFolio(buffer);
            guardado = db.guardarPedido(pedido);
        }

        if (!guardado) return "ERR|No se pudo guardar el pedido.";

        folio = pedido.getFolio();

        // (3) DESCONTAR (matriz + BD) y contabilizar
        for (const auto& item : lista) {
            int f = 0, c = 0;
            obtenerPosicionProducto(item.first.getIdProducto(), f, c);

            inventario[f][c] -= item.second;
            ventas[f][c] += item.second;
            db.actualizarExistencia(item.first.getIdProducto(), inventario[f][c]);
        }

        pedidosPorEstado[fila][indiceEstado("Pendiente")]++;
    }   // <- aqui se sueltan los candados; la bitacora se escribe fuera.

    char totalTexto[32];
    snprintf(totalTexto, sizeof(totalTexto), "%.2f", total);

    registrarEvento("PEDIDO_NUEVO", idCaf,
                    "folio=" + folio + " cliente=" + username + " items=" + detalleItems +
                    " total=$" + totalTexto);

    return "OK|" + folio + "|" + totalTexto;
}

// ---------------------------------------------------------------------
// PAGAR|folio|numeroTarjeta
// ---------------------------------------------------------------------

string Servidor::comandoPagar(const vector<string>& campos) {
    if (campos.size() < 3) return "ERR|Formato invalido. Uso: PAGAR|folio|numeroTarjeta";

    const string& folio = campos[1];
    const string& numeroTarjeta = campos[2];

    string respuesta;
    string tipoEvento;
    string detalleEvento;
    string idCaf;

    {
        scoped_lock lk(dbMutex, mutexInventario);

        Pedido pedido = db.obtenerPedido_Folio(folio);

        if (pedido.getFolio().empty()) return "ERR|Pedido inexistente.";

        idCaf = pedido.getIdCafeteria();

        if (pedido.getEstado() != "Pendiente") {
            return "ERR|El pedido ya no esta pendiente (estado: " + pedido.getEstado() + ").";
        }

        // Evita el DOBLE PAGO: si dos hilos pagan el mismo folio a la vez, el
        // segundo entra aqui despues de que el primero ya guardo su pago.
        if (db.estadoPago(folio) != -1) return "ERR|Este pedido ya tiene un pago registrado.";

        bool aprobado = tarjetaValida(numeroTarjeta);

        Tarjeta tarjeta;
        tarjeta.setNumeroTarjeta(numeroTarjeta);

        Pago pago;
        pago.asignarTarjeta(tarjeta);
        pago.setMonto(pedido.getTotal());
        pago.setAprobado(aprobado);
        pago.setFolioPedido(folio);

        if (!db.guardarPago(pago)) return "ERR|No se pudo registrar el pago.";

        int fila = (idCaf == "1") ? 0 : 1;
        char montoTexto[32];
        snprintf(montoTexto, sizeof(montoTexto), "%.2f", pedido.getTotal());

        if (!aprobado) {
            db.actualizarEstadoPedido(folio, "Cancelado");
            devolverStockPedido(pedido);
            pedidosPorEstado[fila][indiceEstado("Pendiente")]--;
            pedidosPorEstado[fila][indiceEstado("Cancelado")]++;

            tipoEvento = "PAGO_RECHAZADO";
            detalleEvento = "folio=" + folio + " monto=$" + montoTexto + " (pedido cancelado, stock liberado)";
            respuesta = "ERR|Pago rechazado (tarjeta invalida). Pedido cancelado y stock liberado.";
        } else {
            tipoEvento = "PAGO_APROBADO";
            detalleEvento = "folio=" + folio + " monto=$" + montoTexto;
            respuesta = string("OK|APROBADO|") + folio + "|" + montoTexto;
        }
    }

    registrarEvento(tipoEvento, idCaf, detalleEvento);
    return respuesta;
}

// ---------------------------------------------------------------------
// CAMBIAR_ESTADO|folio|nuevoEstado
//   Pendiente -> Preparando (solo si esta pagado) | Cancelado
//   Preparando -> Listo | Cancelado
//   Listo -> Entregado
// ---------------------------------------------------------------------

string Servidor::comandoCambiarEstado(const vector<string>& campos) {
    if (campos.size() < 3) return "ERR|Formato invalido. Uso: CAMBIAR_ESTADO|folio|estado";

    const string& folio = campos[1];
    const string& nuevo = campos[2];

    if (indiceEstado(nuevo) < 0) return "ERR|Estado invalido: " + nuevo;

    string idCaf;
    string anterior;

    {
        scoped_lock lk(dbMutex, mutexInventario);

        Pedido pedido = db.obtenerPedido_Folio(folio);

        if (pedido.getFolio().empty()) return "ERR|Pedido inexistente.";

        anterior = pedido.getEstado();
        idCaf = pedido.getIdCafeteria();

        // La validacion y el cambio son atomicos: dos cafeterias/hilos que
        // intenten avanzar el mismo pedido a la vez no pueden "saltarse" estados.
        bool valida =
            (anterior == "Pendiente" && nuevo == "Preparando") ||
            (anterior == "Pendiente" && nuevo == "Cancelado") ||
            (anterior == "Preparando" && nuevo == "Listo") ||
            (anterior == "Preparando" && nuevo == "Cancelado") ||
            (anterior == "Listo" && nuevo == "Entregado");

        if (!valida) return "ERR|Transicion invalida: " + anterior + " -> " + nuevo;

        if (nuevo == "Preparando" && db.estadoPago(folio) != 1) {
            return "ERR|El pedido aun no tiene un pago aprobado.";
        }

        if (!db.actualizarEstadoPedido(folio, nuevo)) return "ERR|No se pudo actualizar el estado.";

        int fila = (idCaf == "1") ? 0 : 1;
        pedidosPorEstado[fila][indiceEstado(anterior)]--;
        pedidosPorEstado[fila][indiceEstado(nuevo)]++;

        if (nuevo == "Cancelado") devolverStockPedido(pedido);
    }

    registrarEvento("PEDIDO_" + string(nuevo == "Entregado" ? "ENTREGADO" :
                                        nuevo == "Cancelado" ? "CANCELADO" :
                                        nuevo == "Listo" ? "LISTO" : "PREPARANDO"),
                    idCaf, "folio=" + folio + " " + anterior + " -> " + nuevo);

    return "OK|" + folio + "|" + nuevo;
}

// ---------------------------------------------------------------------
// VENTA_CAJA|idProducto|cantidad   (venta directa en el mostrador)
// ---------------------------------------------------------------------

string Servidor::comandoVentaCaja(const vector<string>& campos) {
    if (campos.size() < 3) return "ERR|Formato invalido.";

    const string& idProducto = campos[1];
    int cantidad = 0;

    try {
        cantidad = stoi(campos[2]);
    } catch (...) {
        return "ERR|Cantidad invalida.";
    }

    if (cantidad <= 0 || cantidad > 50) return "ERR|Cantidad fuera de rango.";

    int fila = 0, columna = 0;
    if (!obtenerPosicionProducto(idProducto, fila, columna)) return "ERR|ID de producto invalido.";

    string idCaf(1, idProducto[1]);
    string nombre;
    int nuevoStock = 0;

    {
        scoped_lock lk(dbMutex, mutexInventario);

        bool existe = false;
        for (const auto& p : db.obtenerInventario(idCaf)) {
            if (p.getIdProducto() == idProducto) { existe = true; nombre = p.getNombreProducto(); break; }
        }

        if (!existe) return "ERR|Producto no encontrado.";

        if (inventario[fila][columna] < cantidad) {
            return "ERR|Sin stock suficiente (disponible: " + to_string(inventario[fila][columna]) + ")";
        }

        nuevoStock = inventario[fila][columna] - cantidad;

        if (!db.actualizarExistencia(idProducto, nuevoStock)) return "ERR|No se pudo actualizar el inventario.";

        inventario[fila][columna] = nuevoStock;
        ventas[fila][columna] += cantidad;
    }

    registrarEvento("VENTA_CAJA", idCaf, idProducto + " (" + nombre + ") x" + to_string(cantidad) +
                                          " stock=" + to_string(nuevoStock));

    return "OK|" + idProducto + "|" + to_string(nuevoStock);
}

// ---------------------------------------------------------------------
// RESTOCK|idProducto|cantidad
// ---------------------------------------------------------------------

string Servidor::comandoRestock(const vector<string>& campos) {
    if (campos.size() < 3) return "ERR|Formato invalido.";

    const string& idProducto = campos[1];
    int cantidad = 0;

    try {
        cantidad = stoi(campos[2]);
    } catch (...) {
        return "ERR|Cantidad invalida.";
    }

    if (cantidad <= 0) return "ERR|La cantidad debe ser mayor a cero.";

    int fila = 0, columna = 0;
    if (!obtenerPosicionProducto(idProducto, fila, columna)) {
        return "ERR|ID de producto invalido. Use C1-01 o C2-01.";
    }

    string idCaf(1, idProducto[1]);
    int nuevoStock = 0;

    {
        scoped_lock lk(dbMutex, mutexInventario);

        int stockActual = -1;

        for (const auto& producto : db.obtenerInventario(idCaf)) {
            if (producto.getIdProducto() == idProducto) {
                stockActual = producto.getStock();
                break;
            }
        }

        if (stockActual < 0) return "ERR|Producto no encontrado.";

        nuevoStock = stockActual + cantidad;

        if (!db.actualizarExistencia(idProducto, nuevoStock)) {
            return "ERR|No se pudo actualizar el inventario.";
        }

        actualizarMatrizProducto(idProducto, nuevoStock);
    }

    registrarEvento("RESTOCK", idCaf, idProducto + " +" + to_string(cantidad) + " stock=" + to_string(nuevoStock));

    return "OK|" + idProducto + "|" + to_string(nuevoStock);
}

// ---------------------------------------------------------------------
// Consultas de pedidos
// ---------------------------------------------------------------------

string Servidor::comandoEstadoPedido(const vector<string>& campos) {
    if (campos.size() < 2) return "ERR|Formato invalido.";

    lock_guard<mutex> guard(dbMutex);

    Pedido p = db.obtenerPedido_Folio(campos[1]);

    if (p.getFolio().empty()) return "ERR|Pedido inexistente.";

    char total[32];
    snprintf(total, sizeof(total), "%.2f", p.getTotal());

    return "OK|" + p.getFolio() + "|" + p.getEstado() + "|" + total + "|" +
           (db.estadoPago(p.getFolio()) == 1 ? "1" : "0") + "|" + p.getIdCafeteria() + "|" +
           p.getUsernameCliente();
}

string Servidor::comandoListarPedidos(const vector<string>& campos) {
    if (campos.size() < 2) return "ERR|Formato invalido.";
    if (campos[1] != "1" && campos[1] != "2") return "ERR|Cafeteria invalida.";

    lock_guard<mutex> guard(dbMutex);

    vector<Pedido> pedidos = db.obtenerPedidosCafeteria(campos[1]);

    // solo los ultimos 200 (la pantalla no necesita mas)
    size_t inicio = pedidos.size() > 200 ? pedidos.size() - 200 : 0;

    string respuesta = "OK|" + to_string(pedidos.size() - inicio);

    for (size_t i = inicio; i < pedidos.size(); i++) {
        const Pedido& p = pedidos[i];
        char total[32];
        snprintf(total, sizeof(total), "%.2f", p.getTotal());

        respuesta += "\n" + p.getFolio() + "|" + p.getFecha() + "|" + p.getEstado() + "|" + total + "|" +
                     p.getUsernameCliente() + "|" + (db.estadoPago(p.getFolio()) == 1 ? "1" : "0");
    }

    return respuesta;
}

string Servidor::comandoHistorial(const vector<string>& campos) {
    if (campos.size() < 2) return "ERR|Formato invalido.";

    lock_guard<mutex> guard(dbMutex);

    vector<Pedido> pedidos = db.obtenerHistorialPedidos(campos[1]);
    string respuesta = "OK|" + to_string(pedidos.size());

    for (const auto& p : pedidos) {
        char total[32];
        snprintf(total, sizeof(total), "%.2f", p.getTotal());

        respuesta += "\n" + p.getFolio() + "|" + p.getFecha() + "|" + p.getEstado() + "|" + total + "|" +
                     p.getIdCafeteria();
    }

    return respuesta;
}

string Servidor::comandoTarjetas(const vector<string>& campos) {
    if (campos.size() < 2) return "ERR|Formato invalido.";

    lock_guard<mutex> guard(dbMutex);

    Cliente c = db.obtenerUsuarioCliente(campos[1]);
    vector<Tarjeta> tarjetas = c.getTarjetasGuardadas();

    string respuesta = "OK|" + to_string(tarjetas.size());

    for (const auto& t : tarjetas) {
        respuesta += "\n" + t.getNumeroTarjeta() + "|" + t.getFechaVencimiento();
    }

    return respuesta;
}

// ---------------------------------------------------------------------
// EVENTOS|desdeId[|idCafeteria]     (monitor en vivo de admin / cafeteria)
//   desdeId = -1 -> los ultimos 15 eventos (para dar contexto al abrir el monitor)
// ---------------------------------------------------------------------

string Servidor::comandoEventos(const vector<string>& campos) {
    if (campos.size() < 2) return "ERR|Formato invalido.";

    long desde = 0;

    try {
        desde = stol(campos[1]);
    } catch (...) {
        return "ERR|Id invalido.";
    }

    string filtro = (campos.size() >= 3) ? campos[2] : "";

    vector<Evento> seleccion;
    long ultimo = 0;

    {
        lock_guard<mutex> guard(mutexEventos);
        ultimo = siguienteIdEvento;

        for (const auto& e : eventos) {
            if (!filtro.empty() && e.idCafeteria != filtro) continue;
            if (desde >= 0 && e.id <= desde) continue;
            seleccion.push_back(e);
        }
    }

    size_t maximo = (desde < 0) ? 15 : 100;
    size_t inicio = seleccion.size() > maximo ? seleccion.size() - maximo : 0;

    string respuesta = "OK|" + to_string(seleccion.size() - inicio) + "|" + to_string(ultimo);

    for (size_t i = inicio; i < seleccion.size(); i++) {
        const Evento& e = seleccion[i];
        respuesta += "\n" + to_string(e.id) + "|" + e.hora + "|" + e.tipo + "|" + e.idCafeteria + "|" + e.detalle;
    }

    return respuesta;
}

// ---------------------------------------------------------------------
// Despachador de comandos
// ---------------------------------------------------------------------

string Servidor::procesarComando(const string& comando) {
    vector<string> campos = separarCampos(comando);

    if (campos.empty()) {
        return "ERR|Comando vacio.";
    }

    const string& tipo = campos[0];

    if (tipo == "LOGIN_CLIENTE") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        lock_guard<mutex> guard(dbMutex);

        Cliente c = db.obtenerUsuarioCliente(campos[1]);

        if (c.getUsername().empty()) return "ERR|Usuario inexistente.";
        if (!c.verificarContrasena(campos[2])) return "ERR|Contrasena incorrecta.";

        return "OK|" + c.getNombre() + "|" + c.getCorreo() + "|" +
               c.getApellidoPaterno() + "|" + c.getApellidoMaterno() + "|" +
               c.getTipoCliente() + "|" + c.getUsername();
    }

    if (tipo == "REGISTRO_CLIENTE") {
        if (campos.size() < 8) return "ERR|Formato invalido.";

        Cliente nuevoCliente(campos[2], campos[3], campos[4], campos[1], campos[5], campos[6], campos[7]);

        bool exito;
        {
            lock_guard<mutex> guard(dbMutex);
            exito = db.guardarUsuarioCliente(nuevoCliente);
        }

        if (!exito) return "ERR|No se pudo registrar (el username ya existe?).";

        registrarEvento("REGISTRO", "", "usuario=" + campos[1] + " tipo=" + campos[7]);
        return "OK|" + campos[1];
    }

    if (tipo == "LOGIN_CAFETERIA") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        lock_guard<mutex> guard(dbMutex);

        Cafeteria c = db.obtenerCafeteriaPorUsername(campos[1]);

        if (c.getUsername().empty()) return "ERR|Usuario inexistente.";
        if (!c.verificarContrasena(campos[2])) return "ERR|Contrasena incorrecta.";

        return "OK|" + c.getNombreCafeteria() + "|" + c.getCorreo() + "|" +
               c.getIdCafeteria() + "|" + c.getUsername();
    }

    if (tipo == "LOGIN_ADMIN") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        lock_guard<mutex> guard(dbMutex);

        Usuario u = db.obtenerAdministrador(campos[1]);

        if (u.getUsername().empty()) return "ERR|Usuario inexistente.";
        if (!u.verificarContrasena(campos[2])) return "ERR|Contrasena incorrecta.";

        return "OK|" + u.getNombre() + "|" + u.getCorreo() + "|" + u.getUsername();
    }

    if (tipo == "LISTAR_USUARIOS") {
        lock_guard<mutex> guard(dbMutex);

        vector<Usuario> usuarios = db.obtenerUsuarios();
        string respuesta = "OK|" + to_string(usuarios.size());

        for (const auto& usuario : usuarios) {
            respuesta += "\n" + usuario.getNombre() + "|" + usuario.getUsername() + "|" +
                         usuario.getCorreo() + "|" + usuario.getTipoUsuario();
        }

        return respuesta;
    }

    if (tipo == "LISTAR_CAFETERIAS") {
        lock_guard<mutex> guard(dbMutex);

        vector<Cafeteria> cafeterias = db.obtenerCafeterias();
        string respuesta = "OK|" + to_string(cafeterias.size());

        for (const auto& cafeteria : cafeterias) {
            int cantidadPedidos = db.contarPedidosCafeteria(cafeteria.getIdCafeteria());

            respuesta += "\n" + cafeteria.getIdCafeteria() + "|" + cafeteria.getNombreCafeteria() + "|" +
                         cafeteria.getCorreo() + "|" + cafeteria.getUsername() + "|" +
                         to_string(cantidadPedidos);
        }

        return respuesta;
    }

    if (tipo == "LISTAR_USUARIOS_EN_LINEA") return obtenerUsuariosEnLinea();

    if (tipo == "INVENTARIO") {
        if (campos.size() < 2) return "ERR|Formato invalido.";
        if (campos[1] != "1" && campos[1] != "2") return "ERR|Cafeteria invalida.";

        lock_guard<mutex> guard(dbMutex);

        vector<Producto> productos = db.obtenerInventario(campos[1]);
        string respuesta = "OK|" + to_string(productos.size());

        for (const auto& producto : productos) {
            respuesta += "\n" + producto.getIdProducto() + "|" + producto.getNombreProducto() + "|" +
                         to_string(producto.getStock()) + "|" + to_string(producto.getPrecio());
        }

        return respuesta;
    }

    if (tipo == "RESTOCK") return comandoRestock(campos);
    if (tipo == "VENTA_CAJA") return comandoVentaCaja(campos);
    if (tipo == "CREAR_PEDIDO") return comandoCrearPedido(campos);
    if (tipo == "PAGAR") return comandoPagar(campos);
    if (tipo == "CAMBIAR_ESTADO") return comandoCambiarEstado(campos);
    if (tipo == "ESTADO_PEDIDO") return comandoEstadoPedido(campos);
    if (tipo == "LISTAR_PEDIDOS") return comandoListarPedidos(campos);
    if (tipo == "HISTORIAL") return comandoHistorial(campos);
    if (tipo == "TARJETAS") return comandoTarjetas(campos);
    if (tipo == "EVENTOS") return comandoEventos(campos);
    if (tipo == "ESTADISTICAS") return resumenEstadisticas();

    return "ERR|Comando desconocido: " + tipo;
}
