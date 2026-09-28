#include "Cliente.h"
#include "BaseDatos.h"
#include "Protocolo.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include <cctype>

Cliente::Cliente() {
    tipoCliente = "INVITADO";

    socketCliente = -1;
    ipServidor = "100.70.231.3";
    puerto = 5000;
}

Cliente::Cliente(string nombre, string correo, string contrasena, string username,
                  string apellidoPaterno, string apellidoMaterno, string tipoCliente)
    : Usuario("Cliente", nombre, correo, contrasena, username),
      apellidoPaterno{apellidoPaterno}, apellidoMaterno{apellidoMaterno}, tipoCliente{tipoCliente} {

    socketCliente = -1;
    ipServidor = "100.70.231.3";
    puerto = 5000;
}

Cliente::~Cliente() { }

string Cliente::getApellidoPaterno() const {
    return apellidoPaterno;
}

string Cliente::getApellidoMaterno() const {
    return apellidoMaterno;
}

string Cliente::getTipoCliente() const {
    return tipoCliente;
}

vector<Tarjeta> Cliente::getTarjetasGuardadas() const {
    return tarjetasGuardadas;
}

vector<Pedido> Cliente::getHistorialPedidos() const {
    return historialPedidos;
}

Pedido& Cliente::getPedidoActual() {
    return pedidoActual;
}

void Cliente::setApellidoPaterno(const string& ap) {
    apellidoPaterno = ap;
}

void Cliente::setApellidoMaterno(const string& am) {
    apellidoMaterno = am;
}

void Cliente::setTipoCliente(const string& tc) {
    tipoCliente = tc;
}

void Cliente::setTarjetasGuardadas(const vector<Tarjeta>& tarjetas) {
    tarjetasGuardadas = tarjetas;
}

void Cliente::setHistorialPedidos(const vector<Pedido>& historial) {
    historialPedidos = historial;
}

void Cliente::setPedidoActual(const Pedido& pedido) {
    pedidoActual = pedido;
}

bool Cliente::conectar() {
    std::cout << "Conectando al servidor..." << std::endl;

    socketCliente = socket(AF_INET, SOCK_STREAM, 0);

    if (socketCliente == -1) {
        std::cout << "Error creando socket" << std::endl;
        return false;
    }

    std::cout << "Socket creado correctamente" << std::endl;

    sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(puerto);
    inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr);

    if (connect(socketCliente, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
        std::cout << "Error conectando al servidor" << std::endl;
        return false;
    }

    std::cout << "Conectado al servidor correctamente" << std::endl;

    return true;
}

void Cliente::cargarTarjetasGuardadas() {
    BaseDatos db;

    if (db.conectar()) {
        Cliente actualizado = db.obtenerUsuarioCliente(username);
        tarjetasGuardadas = actualizado.getTarjetasGuardadas();
        db.desconectar();
    }
}

void Cliente::cargarPedidoActual() {
    BaseDatos db;

    if (db.conectar()) {
        pedidoActual = db.obtenerPedido_Username(username);
        db.desconectar();
    }
}

void Cliente::verHistorialP() {
    if (historialPedidos.empty()) {
        cout << "Aun no tienes pedidos registrados." << endl;
        return;
    }

    for (const auto& p : historialPedidos) {
        cout << "Folio: " << p.getFolio()
             << " | Estado: " << p.getEstado()
             << " | Total: $" << p.getTotal() << endl;
    }
}

void Cliente::hacerPedido() {
    pedidoActual.mostrarCarrito();
}

void Cliente::pagar() {
    // La logica de pago (aprobado/rechazado) vive en la clase Pago.
    pedidoActual.generarTicket();
}

void Cliente::recogerPedido() {
    pedidoActual.cambiarEstado("Entregado");
    cout << "Pedido " << pedidoActual.getFolio() << " entregado." << endl;
}

void Cliente::verEstadoPedido() {
    if (pedidoActual.getFolio().empty()) {
        cout << "No tienes un pedido activo." << endl;
        return;
    }

    cout << "Pedido #" << pedidoActual.getFolio()
         << " -> Estado: " << pedidoActual.getEstado() << endl;
}

// ---------------------------------------------------------------------
// Registro / inicio de sesion
// ---------------------------------------------------------------------

bool Cliente::registrarNuevoCliente() {
    string correo, nom, apellidoP, apellidoM, escuela, contrasena;
    int anio;

    cout << "==================================" << endl;
    cout << "Registro de usuario" << endl;

    cout << "Ingrese su correo institucional: ";
    cin >> correo;

    cout << "Ingrese su nombre SIN APELLIDOS: ";
    cin >> nom;

    cout << "Ingrese su apellido paterno: ";
    cin >> apellidoP;

    cout << "Ingrese su apellido materno: ";
    cin >> apellidoM;

    cout << "Ingrese el anio en el que se unio al IPN: ";
    cin >> anio;

    cout << "Ingrese el nombre de la escuela de su procedencia: ";
    cin >> escuela;

    // La validacion del correo/escuela es una regla de negocio pura, no
    // necesita al servidor: se valida aqui mismo antes de gastar una vuelta
    // de red.
    if (!validarIPN(correo, anio, escuela)) {
        cout << "No es posible registrar el usuario." << endl;
        cout << "El correo institucional o la escuela no son validos." << endl;
        return false;
    }

    string username = asignarUsername(correo);

    cout << "Registro realizado correctamente." << endl;
    cout << "Username asignado: " << username << endl;
    cout << "Ingrese su contrasena (minimo 8 caracteres): ";
    cin >> contrasena;

    string tipoCliente = (normalizarTexto(escuela) == "UPIITA") ? "UPIITA" : "IPN";

    // REGISTRO_CLIENTE|username|nombre|correo|contrasena|apellidoP|apellidoM|tipoCliente
    string comando = "REGISTRO_CLIENTE|" + username + "|" + nom + "|" + correo + "|" +
                      contrasena + "|" + apellidoP + "|" + apellidoM + "|" + tipoCliente;

    string respuesta = enviarComando(socketCliente, comando);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        string motivo = (campos.size() > 1) ? campos[1] : "No se pudo registrar.";
        cout << motivo << endl;
        return false;
    }

    return true;
}

bool Cliente::iniciarSesionCliente() {
    cout << "==================================" << endl;
    cout << "Inicio de sesion" << endl;

    string username, contrasena;

    cout << "Ingrese username: ";
    cin >> username;

    cout << "Ingrese contrasena: ";
    cin >> contrasena;

    // LOGIN_CLIENTE|username|contrasena
    string respuesta = enviarComando(socketCliente, "LOGIN_CLIENTE|" + username + "|" + contrasena);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        string motivo = (campos.size() > 1) ? campos[1] : "Error desconocido.";
        cout << motivo << endl;
        return false;
    }

    // Respuesta esperada: OK|nombre|correo|apellidoP|apellidoM|tipoCliente|username
    if (campos.size() < 7) {
        cout << "Respuesta invalida del servidor." << endl;
        return false;
    }

    setNombre(campos[1]);
    setCorreo(campos[2]);
    setApellidoPaterno(campos[3]);
    setApellidoMaterno(campos[4]);
    setTipoCliente(campos[5]);
    setUsername(campos[6]);
    setTipoUsuario("Cliente");

    cout << "Inicio de sesion correcto." << endl;
    return true;
}

// ---------------------------------------------------------------------
// Validaciones IPN
// ---------------------------------------------------------------------

bool Cliente::validarIPN(const string& correo, int anio, const string& escuela) {
    if (correo.empty()) {
        return false;
    }

    // 2014: aproximadamente 12 anios maximo para terminar una carrera / seguir vigente
    if (anio <= 2014) {
        return false;
    }

    if (!validarEscuela(escuela)) {
        return false;
    }

    size_t posArroba = correo.find('@');
    if (posArroba == string::npos) {
        return false;
    }

    string dominio = correo.substr(posArroba);
    if (dominio != "@alumno.ipn.mx") {
        return false;
    }

    return true;
}

bool Cliente::validarEscuela(const string& escuela) {
    vector<string> escuelasIPN = {"ESIME", "ESIA", "ESIQUIE", "ESIQ", "ESIT", "ESCOM", "UPIITA", "UPIBI",
                                   "UPIIG", "UPIIZ", "UPIIH", "UPIIT", "UPIIP", "UPII", "UPIICSA", "ENCB",
                                   "ESM", "CICS", "ESCA", "ESE", "EST", "ESFM", "ESEO", "UPIIC", "UPIIY",
                                   "ENMH", "ENBA"};

    string escuelaNormalizada = normalizarTexto(escuela);

    for (const string& escuelaIPN : escuelasIPN) {
        if (normalizarTexto(escuelaIPN) == escuelaNormalizada) {
            return true;
        }
    }

    return false;
}

string Cliente::normalizarTexto(const string& texto) {
    string resultado = "";

    for (char c : texto) {
        if (!isspace((unsigned char)c)) {
            resultado += toupper((unsigned char)c);
        }
    }

    return resultado;
}

string Cliente::asignarUsername(const string& correo) {
    size_t posArroba = correo.find('@');

    if (posArroba == string::npos) {
        return correo;
    }

    return correo.substr(0, posArroba);
}

void Cliente::setIpServidor(const string& ip) {
    ipServidor = ip;
}
