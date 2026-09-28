#include "Servidor.h"
#include "Protocolo.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Usuario.h"
#include "Producto.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <csignal>
#include <signal.h>
#include <cerrno>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <poll.h>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <cstdlib>

using namespace std;

namespace {

volatile sig_atomic_t detenerServidor = 0;
volatile sig_atomic_t solicitarPedidoSimulado = 0;
volatile sig_atomic_t alternarSimulacion = 0;
atomic<unsigned long long> secuenciaFolios{0};

void manejarSenalServidor(int senal) {
    if (senal == SIGINT || senal == SIGTERM) {
        detenerServidor = 1;
    } else if (senal == SIGUSR1) {
        solicitarPedidoSimulado = 1;
    } else if (senal == SIGUSR2) {
        alternarSimulacion = 1;
    }
}

struct SolicitudSimulada {
    string username;
    string contrasena;
    string idCafeteria;
    string idProducto;
};

bool escribirCompleto(int descriptor, const string& datos) {
    size_t enviados = 0;
    while (enviados < datos.size()) {
        ssize_t resultado = write(descriptor, datos.data() + enviados, datos.size() - enviados);
        if (resultado < 0 && errno == EINTR) continue;
        if (resultado <= 0) return false;
        enviados += static_cast<size_t>(resultado);
    }
    return true;
}

bool leerLineaPipe(int descriptor, string& linea) {
    linea.clear();
    char byte = '\0';
    while (true) {
        ssize_t resultado = read(descriptor, &byte, 1);
        if (resultado < 0 && errno == EINTR) continue;
        if (resultado <= 0) return !linea.empty();
        if (byte == '\n') return true;
        linea += byte;
    }
}

vector<string> dividir(const string& texto, char separador) {
    vector<string> resultado;
    string campo;
    stringstream flujo(texto);
    while (getline(flujo, campo, separador)) resultado.push_back(campo);
    return resultado;
}

void ejecutarClienteSimulado(const SolicitudSimulada& solicitud) {
    int socketCliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socketCliente == -1) return;

    sockaddr_in direccion{};
    direccion.sin_family = AF_INET;
    direccion.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &direccion.sin_addr);

    if (connect(socketCliente, reinterpret_cast<sockaddr*>(&direccion), sizeof(direccion)) == -1) {
        close(socketCliente);
        return;
    }

    string respuesta = enviarComando(socketCliente,
        "LOGIN_CLIENTE|" + solicitud.username + "|" + solicitud.contrasena);
    vector<string> camposLogin = separarCampos(respuesta);
    if (!camposLogin.empty() && camposLogin[0] == "OK") {
        respuesta = enviarComando(socketCliente,
            "CREAR_PEDIDO|" + solicitud.idCafeteria + "|" + solicitud.idProducto + "|1");
        cout << "[SIMULADOR] " << solicitud.username << " -> " << respuesta << endl;
    }

    close(socketCliente);
}

void ejecutarProcesoSimulacion(int pipeLectura, int socketEscucha) {
    close(socketEscucha);
    signal(SIGPIPE, SIG_IGN);

    string mensaje;
    while (leerLineaPipe(pipeLectura, mensaje)) {
        if (mensaje == "STOP") break;
        vector<string> registros = dividir(mensaje, ';');
        vector<thread> clientesSimulados;

        for (const string& registro : registros) {
            vector<string> campos = dividir(registro, ',');
            if (campos.size() != 4) continue;
            SolicitudSimulada solicitud{campos[0], campos[1], campos[2], campos[3]};
            clientesSimulados.emplace_back([solicitud]() { ejecutarClienteSimulado(solicitud); });
        }

        for (thread& cliente : clientesSimulados) {
            if (cliente.joinable()) cliente.join();
        }
    }

    close(pipeLectura);
    _exit(0);
}

string fechaActual() {
    time_t ahora = time(nullptr);
    tm tiempoLocal{};
    localtime_r(&ahora, &tiempoLocal);
    stringstream salida;
    salida << put_time(&tiempoLocal, "%Y-%m-%d %H:%M:%S");
    return salida.str();
}

} // namespace

Servidor::Servidor() {
    socketServidor = -1;
    puerto = 5000;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            inventario[i][j] = 0;
        }
    }
}

bool Servidor::iniciar() {
    if (!db.conectar()) {
        cout << "No se pudo abrir la base de datos. El servidor no puede iniciar." << endl;
        return false;
    }

    vector<string> usuariosSimulados;
    {
        lock_guard<mutex> guardDb(dbMutex);
        for (int i = 1; i <= 3; ++i) {
            string username = "SIM-CLIENTE-0" + to_string(i);
            Cliente usuarioSimulado("Cliente simulado " + to_string(i),
                "simulado" + to_string(i) + "@upifood.local", "SIMULACION123",
                username, "", "", "EXTERNO");

            Cliente clienteExistente = db.obtenerUsuarioCliente(username);
            if (clienteExistente.getUsername().empty()) {
                if (!db.guardarUsuarioCliente(usuarioSimulado)) {
                    cout << "No se pudo preparar el cliente simulado " << username << "." << endl;
                    continue;
                }
            } else if (!clienteExistente.verificarContrasena("SIMULACION123")) {
                cout << "El username de simulacion " << username
                     << " ya existe con otra contrasena; se omitira." << endl;
                continue;
            }
            usuariosSimulados.push_back(username);
        }
    }
    // El proceso de simulación no debe heredar una conexión SQLite abierta.
    db.desconectar();

    socketServidor = socket(AF_INET, SOCK_STREAM, 0);

    if (socketServidor == -1) {
        cout << "Error creando el socket del servidor." << endl;
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
        return false;
    }

    if (listen(socketServidor, 10) == -1) {
        cout << "Error poniendo el socket en modo escucha." << endl;
        return false;
    }

    cout << "Servidor escuchando en el puerto " << puerto << "..." << endl;
    cout << "Simulador: genera hasta 3 clientes concurrentes cada 8 segundos cuando hay stock." << endl;
    cout << "Senales: SIGUSR1 genera ahora; SIGUSR2 pausa/reanuda; SIGINT o SIGTERM apagan." << endl;
    cout << "PID del servidor: " << getpid() << endl;

    struct sigaction accion{};
    accion.sa_handler = manejarSenalServidor;
    sigemptyset(&accion.sa_mask);
    accion.sa_flags = 0;
    sigaction(SIGINT, &accion, nullptr);
    sigaction(SIGTERM, &accion, nullptr);
    sigaction(SIGUSR1, &accion, nullptr);
    sigaction(SIGUSR2, &accion, nullptr);
    signal(SIGPIPE, SIG_IGN);
    detenerServidor = 0;
    solicitarPedidoSimulado = 0;
    alternarSimulacion = 0;

    int descriptoresPipe[2] = {-1, -1};
    pid_t procesoSimulador = -1;
    thread hiloProgramador;
    int pipeEscritura = -1;

    if (pipe(descriptoresPipe) == 0) {
        procesoSimulador = fork();
        if (procesoSimulador == 0) {
            close(descriptoresPipe[1]);
            ejecutarProcesoSimulacion(descriptoresPipe[0], socketServidor);
        } else if (procesoSimulador > 0) {
            close(descriptoresPipe[0]);
            pipeEscritura = descriptoresPipe[1];
        } else {
            cerr << "No se pudo crear el proceso simulador con fork()." << endl;
            close(descriptoresPipe[0]);
            close(descriptoresPipe[1]);
        }
    } else {
        cerr << "No se pudo crear el pipe del simulador." << endl;
    }

    if (!db.conectar()) {
        cerr << "No se pudo reabrir la base de datos en el servidor padre." << endl;
        if (pipeEscritura != -1) {
            escribirCompleto(pipeEscritura, "STOP\n");
            close(pipeEscritura);
        }
        close(socketServidor);
        socketServidor = -1;
        if (procesoSimulador > 0) {
            int estadoProceso = 0;
            waitpid(procesoSimulador, &estadoProceso, 0);
        }
        return false;
    }
    cargarMatrizInventario();

    if (procesoSimulador > 0) {
        hiloProgramador = thread([this, pipeEscritura, usuariosSimulados]() {
            bool simulacionActiva = true;
            bool avisoSinInventario = false;
            int ciclosRestantes = 32;

            while (!detenerServidor) {
                if (alternarSimulacion) {
                    alternarSimulacion = 0;
                    simulacionActiva = !simulacionActiva;
                    cout << "[SIMULADOR] " << (simulacionActiva ? "Reanudado" : "Pausado") << endl;
                }

                bool generarAhora = solicitarPedidoSimulado != 0;
                if (generarAhora) solicitarPedidoSimulado = 0;

                    bool vencioIntervalo = simulacionActiva && --ciclosRestantes <= 0;
                    if (generarAhora || vencioIntervalo) {
                    ciclosRestantes = 32;
                    vector<pair<string, string>> productosDisponibles;
                    {
                        lock_guard<mutex> guardDb(dbMutex);
                        vector<Producto> inventarioCafeteria1 = db.obtenerInventario("1");
                        vector<Producto> inventarioCafeteria2 = db.obtenerInventario("2");
                        size_t maxProductos = max(inventarioCafeteria1.size(), inventarioCafeteria2.size());
                        for (size_t i = 0; i < maxProductos; ++i) {
                            if (i < inventarioCafeteria1.size() && inventarioCafeteria1[i].getStock() > 0) {
                                productosDisponibles.push_back({"1", inventarioCafeteria1[i].getIdProducto()});
                            }
                            if (i < inventarioCafeteria2.size() && inventarioCafeteria2[i].getStock() > 0) {
                                productosDisponibles.push_back({"2", inventarioCafeteria2[i].getIdProducto()});
                            }
                        }
                    }

                    if (usuariosSimulados.empty() || productosDisponibles.empty()) {
                        if (!avisoSinInventario) {
                            cout << "[SIMULADOR] Sin usuarios demo disponibles o inventario con stock; "
                                    "agrega cafeterias/productos para generar pedidos." << endl;
                            avisoSinInventario = true;
                        }
                    } else {
                        avisoSinInventario = false;
                        string mensaje;
                        size_t cantidad = min(usuariosSimulados.size(), productosDisponibles.size());
                        for (size_t i = 0; i < cantidad; ++i) {
                            if (!mensaje.empty()) mensaje += ';';
                            const auto& producto = productosDisponibles[i];
                            mensaje += usuariosSimulados[i] + ",SIMULACION123," +
                                       producto.first + "," + producto.second;
                        }
                        if (!escribirCompleto(pipeEscritura, mensaje + "\n")) {
                            cerr << "[SIMULADOR] Se perdió el canal pipe al proceso hijo." << endl;
                            break;
                        }
                        cout << "[SIMULADOR] Generando " << cantidad
                             << " clientes concurrentes." << endl;
                    }
                }

                this_thread::sleep_for(chrono::milliseconds(250));
            }
        });
    }

    while (!detenerServidor) {
        for (auto it = hilosClientes.begin(); it != hilosClientes.end();) {
            if (it->terminado->load()) {
                if (it->hilo.joinable()) it->hilo.join();
                it = hilosClientes.erase(it);
            } else {
                ++it;
            }
        }

        pollfd descriptorEscucha{};
        descriptorEscucha.fd = socketServidor;
        descriptorEscucha.events = POLLIN;
        int disponible = poll(&descriptorEscucha, 1, 500);

        if (disponible < 0) {
            if (errno == EINTR) continue;
            cerr << "Error esperando conexiones." << endl;
            break;
        }
        if (disponible == 0) continue;
        if ((descriptorEscucha.revents & POLLIN) == 0) continue;

        sockaddr_in direccionCliente;
        socklen_t tam = sizeof(direccionCliente);

        int socketCliente = accept(socketServidor, (sockaddr*)&direccionCliente, &tam);

        if (socketCliente == -1) {
            if (errno == EINTR) continue;
            if (!detenerServidor) cout << "Error aceptando una conexion." << endl;
            continue;
        }

        char ipCliente[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &direccionCliente.sin_addr, ipCliente, sizeof(ipCliente));
        cout << "Nueva conexion aceptada desde " << ipCliente << "." << endl;

        {
            lock_guard<mutex> guard(mutexSocketsClientes);
            socketsClientes.push_back(socketCliente);
        }
        auto terminado = make_shared<atomic<bool>>(false);
        thread hilo([this, socketCliente, ip = string(ipCliente), terminado]() {
            atenderCliente(socketCliente, ip);
            terminado->store(true);
        });
        hilosClientes.push_back({move(hilo), terminado});
    }

    detenerServidor = 1;
    if (hiloProgramador.joinable()) hiloProgramador.join();

    close(socketServidor);
    socketServidor = -1;

    {
        lock_guard<mutex> guard(mutexSocketsClientes);
        for (int socketCliente : socketsClientes) shutdown(socketCliente, SHUT_RDWR);
    }
    for (HiloClienteControl& control : hilosClientes) {
        if (control.hilo.joinable()) control.hilo.join();
    }
    hilosClientes.clear();

    if (procesoSimulador > 0) {
        escribirCompleto(descriptoresPipe[1], "STOP\n");
        close(descriptoresPipe[1]);
        int estadoProceso = 0;
        waitpid(procesoSimulador, &estadoProceso, 0);
    }
    cout << "Servidor detenido de forma segura." << endl;

    return true;
}

void Servidor::atenderCliente(int socketCliente, const string& ipCliente) {
    string usuarioConectado;
    string tipoUsuarioConectado;
    string idCafeteriaConectada;

    while (true) {
        string comando;

        if (!recibirMensaje(socketCliente, comando)) {
            break;
        }

        cout << "Comando recibido: " << comando << endl;

        string respuesta = procesarComando(comando, usuarioConectado,
                           tipoUsuarioConectado, idCafeteriaConectada);

        vector<string> camposComando = separarCampos(comando);
        vector<string> camposRespuesta = separarCampos(respuesta);

        if (!camposComando.empty() && !camposRespuesta.empty() && camposRespuesta[0] == "OK") {
            const string& tipo = camposComando[0];

            if (tipo == "LOGIN_CLIENTE" && camposRespuesta.size() >= 7) {
                const string& nuevoUsername = camposRespuesta[6];
                if (usuarioConectado != nuevoUsername) {
                    if (!usuarioConectado.empty()) marcarUsuarioFueraDeLinea(usuarioConectado);
                    usuarioConectado = nuevoUsername;
                    marcarUsuarioEnLinea(usuarioConectado);
                }
                tipoUsuarioConectado = "Cliente";
                idCafeteriaConectada.clear();
            }
            else if (tipo == "LOGIN_CAFETERIA" && camposRespuesta.size() >= 5) {
                const string& nuevoUsername = camposRespuesta[4];
                if (usuarioConectado != nuevoUsername) {
                    if (!usuarioConectado.empty()) marcarUsuarioFueraDeLinea(usuarioConectado);
                    usuarioConectado = nuevoUsername;
                    marcarUsuarioEnLinea(usuarioConectado);
                }
                tipoUsuarioConectado = "Cafe";
                idCafeteriaConectada = camposRespuesta[3];
            }
            else if (tipo == "LOGIN_ADMIN" && camposRespuesta.size() >= 4) {
                const string& nuevoUsername = camposRespuesta[3];
                if (usuarioConectado != nuevoUsername) {
                    if (!usuarioConectado.empty()) marcarUsuarioFueraDeLinea(usuarioConectado);
                    usuarioConectado = nuevoUsername;
                    marcarUsuarioEnLinea(usuarioConectado);
                }
                tipoUsuarioConectado = "Admin";
                idCafeteriaConectada.clear();
            }
        }

        if (!enviarMensaje(socketCliente, respuesta)) {
            break;
        }
    }

    if (!usuarioConectado.empty()) {
        marcarUsuarioFueraDeLinea(usuarioConectado);
    }

    close(socketCliente);
    {
        lock_guard<mutex> guard(mutexSocketsClientes);
        socketsClientes.erase(remove(socketsClientes.begin(), socketsClientes.end(), socketCliente),
                              socketsClientes.end());
    }
    cout << "Conexion cerrada desde " << ipCliente << "." << endl;
}

void Servidor::cargarMatrizInventario() {
    lock_guard<mutex> guardDb(dbMutex);
    lock_guard<mutex> guardInventario(mutexInventario);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            inventario[i][j] = 0;
        }
    }

    for (int cafeteria = 1; cafeteria <= 2; cafeteria++) {
        vector<Producto> productos = db.obtenerInventario(to_string(cafeteria));

        for (const auto& producto : productos) {
            int fila = 0;
            int columna = 0;

            if (obtenerPosicionProducto(producto.getIdProducto(), fila, columna)) {
                inventario[fila][columna] = producto.getStock();
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

    lock_guard<mutex> guard(mutexInventario);
    inventario[fila][columna] = stock;
    return true;
}

void Servidor::marcarUsuarioEnLinea(const string& username) {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);
    ++conexionesUsuariosEnLinea[username];
}

void Servidor::marcarUsuarioFueraDeLinea(const string& username) {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    auto conexion = conexionesUsuariosEnLinea.find(username);
    if (conexion == conexionesUsuariosEnLinea.end()) return;
    if (conexion->second > 1) {
        --conexion->second;
    } else {
        conexionesUsuariosEnLinea.erase(conexion);
    }
}

string Servidor::obtenerUsuariosEnLinea() {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    string respuesta = "OK|" + to_string(conexionesUsuariosEnLinea.size());

    for (const auto& [username, conexiones] : conexionesUsuariosEnLinea) {
        (void)conexiones;
        respuesta += "\n" + username;
    }

    return respuesta;
}

string Servidor::procesarComando(const string& comando, const string& usernameConectado,
                                 const string& tipoUsuarioConectado,
                                 const string& idCafeteriaConectada) {
    vector<string> campos = separarCampos(comando);

    if (campos.empty()) {
        return "ERR|Comando vacio.";
    }

    const string& tipo = campos[0];

    // ---------------------------------------------------------------
    // LOGIN_CLIENTE|username|contrasena
    // ---------------------------------------------------------------
    if (tipo == "LOGIN_CLIENTE") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string username = campos[1];
        string contrasena = campos[2];

        lock_guard<mutex> guard(dbMutex);

        Cliente c = db.obtenerUsuarioCliente(username);

        if (c.getUsername().empty()) {
            return "ERR|Usuario inexistente.";
        }

        if (!c.verificarContrasena(contrasena)) {
            return "ERR|Contrasena incorrecta.";
        }

        return "OK|" + c.getNombre() + "|" + c.getCorreo() + "|" +
               c.getApellidoPaterno() + "|" + c.getApellidoMaterno() + "|" +
               c.getTipoCliente() + "|" + c.getUsername();
    }

    // ---------------------------------------------------------------
    // REGISTRO_CLIENTE|username|nombre|correo|contrasena|apellidoP|apellidoM|tipoCliente
    // ---------------------------------------------------------------
    if (tipo == "REGISTRO_CLIENTE") {
        if (campos.size() < 8) return "ERR|Formato invalido.";

        string username = campos[1];
        string nombre = campos[2];
        string correo = campos[3];
        string contrasena = campos[4];
        string apellidoP = campos[5];
        string apellidoM = campos[6];
        string tipoCliente = campos[7];

        Cliente nuevoCliente(nombre, correo, contrasena, username, apellidoP, apellidoM, tipoCliente);

        lock_guard<mutex> guard(dbMutex);

        bool exito = db.guardarUsuarioCliente(nuevoCliente);

        if (!exito) {
            return "ERR|No se pudo registrar (el username ya existe?).";
        }

        return "OK|" + username;
    }

    // ---------------------------------------------------------------
    // LOGIN_CAFETERIA|username|contrasena
    // ---------------------------------------------------------------
    if (tipo == "LOGIN_CAFETERIA") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string username = campos[1];
        string contrasena = campos[2];

        lock_guard<mutex> guard(dbMutex);

        Cafeteria c = db.obtenerCafeteriaPorUsername(username);

        if (c.getUsername().empty()) {
            return "ERR|Usuario inexistente.";
        }

        if (!c.verificarContrasena(contrasena)) {
            return "ERR|Contrasena incorrecta.";
        }

        return "OK|" + c.getNombreCafeteria() + "|" + c.getCorreo() + "|" +
               c.getIdCafeteria() + "|" + c.getUsername();
    }

    // ---------------------------------------------------------------
    // LOGIN_ADMIN|username|contrasena
    // ---------------------------------------------------------------
    if (tipo == "LOGIN_ADMIN") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string username = campos[1];
        string contrasena = campos[2];

        lock_guard<mutex> guard(dbMutex);

        Usuario u = db.obtenerAdministrador(username);

        if (u.getUsername().empty()) {
            return "ERR|Usuario inexistente.";
        }

        if (!u.verificarContrasena(contrasena)) {
            return "ERR|Contrasena incorrecta.";
        }

        return "OK|" + u.getNombre() + "|" + u.getCorreo() + "|" + u.getUsername();
    }

    // ---------------------------------------------------------------
    // LISTAR_USUARIOS
    // ---------------------------------------------------------------
    if (tipo == "LISTAR_USUARIOS") {
        if (tipoUsuarioConectado != "Admin") return "ERR|Se requiere una sesion de administrador.";
        lock_guard<mutex> guard(dbMutex);

        vector<Usuario> usuarios = db.obtenerUsuarios();
        string respuesta = "OK|" + to_string(usuarios.size());

        for (const auto& usuario : usuarios) {
            respuesta += "\n" + usuario.getNombre() + "|" +
                         usuario.getUsername() + "|" +
                         usuario.getCorreo() + "|" +
                         usuario.getTipoUsuario();
        }

        return respuesta;
    }

    // ---------------------------------------------------------------
    // LISTAR_CAFETERIAS
    // ---------------------------------------------------------------
    if (tipo == "LISTAR_CAFETERIAS") {
        if (tipoUsuarioConectado != "Admin") return "ERR|Se requiere una sesion de administrador.";
        lock_guard<mutex> guard(dbMutex);

        vector<Cafeteria> cafeterias = db.obtenerCafeterias();
        string respuesta = "OK|" + to_string(cafeterias.size());

        for (const auto& cafeteria : cafeterias) {
            int cantidadPedidos = db.contarPedidosCafeteria(cafeteria.getIdCafeteria());

            respuesta += "\n" + cafeteria.getIdCafeteria() + "|" +
                         cafeteria.getNombreCafeteria() + "|" +
                         cafeteria.getCorreo() + "|" +
                         cafeteria.getUsername() + "|" +
                         to_string(cantidadPedidos);
        }

        return respuesta;
    }

    // ---------------------------------------------------------------
    // LISTAR_USUARIOS_EN_LINEA
    // ---------------------------------------------------------------
    if (tipo == "LISTAR_USUARIOS_EN_LINEA") {
        if (tipoUsuarioConectado != "Admin") return "ERR|Se requiere una sesion de administrador.";
        return obtenerUsuariosEnLinea();
    }

    // ---------------------------------------------------------------
    // INVENTARIO|idCafeteria
    // ---------------------------------------------------------------
    if (tipo == "INVENTARIO") {
        if (campos.size() < 2) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];

        if (tipoUsuarioConectado != "Cafe" || idCafeteria != idCafeteriaConectada) {
            return "ERR|Cafeteria invalida.";
        }

        lock_guard<mutex> guard(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idCafeteria);
        string respuesta = "OK|" + to_string(productos.size());

        for (const auto& producto : productos) {
            respuesta += "\n" + producto.getIdProducto() + "|" +
                         producto.getNombreProducto() + "|" +
                         to_string(producto.getStock()) + "|" +
                         to_string(producto.getPrecio());
        }

        return respuesta;
    }

    // CREAR_PEDIDO|idCafeteria|idProducto|cantidad
    if (tipo == "CREAR_PEDIDO") {
        if (tipoUsuarioConectado != "Cliente" || usernameConectado.empty()) {
            return "ERR|Solo un cliente autenticado puede crear pedidos.";
        }
        if (campos.size() < 4) return "ERR|Formato invalido.";

        const string& idCafeteria = campos[1];
        const string& idProducto = campos[2];
        int cantidad = 0;
        try {
            cantidad = stoi(campos[3]);
        } catch (...) {
            return "ERR|Cantidad invalida.";
        }
        if ((idCafeteria != "1" && idCafeteria != "2") || cantidad <= 0) {
            return "ERR|Cafeteria o cantidad invalida.";
        }

        int filaInventario = 0;
        int columnaInventario = 0;
        if (!obtenerPosicionProducto(idProducto, filaInventario, columnaInventario) ||
            idProducto[1] != idCafeteria[0]) {
            return "ERR|El producto no pertenece a esa cafeteria.";
        }

        lock_guard<mutex> guardDb(dbMutex);
        {
            lock_guard<mutex> guardInventario(mutexInventario);
            if (inventario[filaInventario][columnaInventario] < cantidad) {
                return "ERR|La matriz indica que no hay suficiente inventario.";
            }
        }
        vector<Producto> inventarioCafeteria = db.obtenerInventario(idCafeteria);
        auto encontrado = find_if(inventarioCafeteria.begin(), inventarioCafeteria.end(),
            [&idProducto](const Producto& producto) { return producto.getIdProducto() == idProducto; });
        if (encontrado == inventarioCafeteria.end()) return "ERR|Producto no encontrado en esa cafeteria.";
        if (encontrado->getStock() < cantidad) return "ERR|No hay suficiente inventario.";

        vector<pair<Producto, int>> detalle = {{*encontrado, cantidad}};
        float total = encontrado->getPrecio() * cantidad;
        auto ahora = chrono::system_clock::now().time_since_epoch();
        auto milisegundos = chrono::duration_cast<chrono::milliseconds>(ahora).count();
        string folio = "UPI-" + to_string(milisegundos) + "-" +
                       to_string(secuenciaFolios.fetch_add(1));
        Pedido pedido(folio, usernameConectado, idCafeteria, total, detalle);
        pedido.setFecha(fechaActual());
        pedido.setEstado("Pendiente");

        if (!db.guardarPedido(pedido)) return "ERR|No se pudo registrar el pedido o el inventario cambio.";

        actualizarMatrizProducto(idProducto, encontrado->getStock() - cantidad);
        cout << "[PEDIDO NUEVO] " << folio << " | Cliente " << usernameConectado
             << " | Cafeteria " << idCafeteria << " | $" << total << endl;
        return "OK|" + folio + "|" + pedido.getFecha() + "|Pendiente|" + to_string(total);
    }

    // Listados devuelven primero OK|N y luego N frames con resumenes.
    if (tipo == "LISTAR_PEDIDOS_CAFETERIA") {
        if (tipoUsuarioConectado != "Cafe" || idCafeteriaConectada.empty()) {
            return "ERR|Se requiere una sesion de cafeteria.";
        }

        lock_guard<mutex> guardDb(dbMutex);
        vector<Pedido> pedidos = db.obtenerPedidosCafeteria(idCafeteriaConectada);
        string respuesta = "OK|" + to_string(pedidos.size());
        for (const Pedido& pedido : pedidos) {
            respuesta += "\n" + pedido.getFolio() + "|" + pedido.getFecha() + "|" +
                         pedido.getEstado() + "|" + to_string(pedido.getTotal()) + "|" +
                         pedido.getUsernameCliente() + "|" + pedido.getIdCafeteria();
        }
        return respuesta;
    }

    if (tipo == "LISTAR_PEDIDOS_ADMIN") {
        if (tipoUsuarioConectado != "Admin") return "ERR|Se requiere una sesion de administrador.";

        lock_guard<mutex> guardDb(dbMutex);
        vector<Pedido> pedidos = db.obtenerTodosPedidos();
        string respuesta = "OK|" + to_string(pedidos.size());
        for (const Pedido& pedido : pedidos) {
            respuesta += "\n" + pedido.getFolio() + "|" + pedido.getFecha() + "|" +
                         pedido.getEstado() + "|" + to_string(pedido.getTotal()) + "|" +
                         pedido.getUsernameCliente() + "|" + pedido.getIdCafeteria();
        }
        return respuesta;
    }

    // ACTUALIZAR_PEDIDO|folio|Preparando|Listo|Entregado|Cancelado
    if (tipo == "ACTUALIZAR_PEDIDO") {
        if (tipoUsuarioConectado != "Cafe" || idCafeteriaConectada.empty()) {
            return "ERR|Se requiere una sesion de cafeteria.";
        }
        if (campos.size() < 3) return "ERR|Formato invalido.";

        lock_guard<mutex> guardDb(dbMutex);
        if (!db.actualizarEstadoPedido(campos[1], idCafeteriaConectada, campos[2])) {
            return "ERR|No se pudo actualizar el estado del pedido.";
        }
        cout << "[ESTADO PEDIDO] " << campos[1] << " -> " << campos[2] << endl;
        return "OK|" + campos[1] + "|" + campos[2];
    }

    // ---------------------------------------------------------------
    // RESTOCK|idProducto|cantidad
    // ---------------------------------------------------------------
    if (tipo == "RESTOCK") {
        if (tipoUsuarioConectado != "Cafe" || idCafeteriaConectada.empty()) {
            return "ERR|Se requiere una sesion de cafeteria.";
        }
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string idProducto = campos[1];
        if (idProducto.size() < 2 || idProducto[1] != idCafeteriaConectada[0]) {
            return "ERR|El producto no pertenece a esta cafeteria.";
        }
        int cantidad = 0;

        try {
            cantidad = stoi(campos[2]);
        } catch (...) {
            return "ERR|Cantidad invalida.";
        }

        if (cantidad <= 0) {
            return "ERR|La cantidad debe ser mayor a cero.";
        }

        int fila = 0;
        int columna = 0;

        if (!obtenerPosicionProducto(idProducto, fila, columna)) {
            return "ERR|ID de producto invalido. Use C1-01 o C2-01.";
        }

        lock_guard<mutex> guardDb(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idProducto.substr(1, 1));
        int stockActual = -1;

        for (const auto& producto : productos) {
            if (producto.getIdProducto() == idProducto) {
                stockActual = producto.getStock();
                break;
            }
        }

        if (stockActual < 0) {
            return "ERR|Producto no encontrado.";
        }

        int nuevoStock = stockActual + cantidad;

        if (!db.actualizarExistencia(idProducto, nuevoStock)) {
            return "ERR|No se pudo actualizar el inventario.";
        }

        actualizarMatrizProducto(idProducto, nuevoStock);

        return "OK|" + idProducto + "|" + to_string(nuevoStock);
    }

    return "ERR|Comando desconocido: " + tipo;
}
