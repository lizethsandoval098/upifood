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
#include <cstdio>
#include <stdexcept>
#include <ctime>

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

Cliente::~Cliente() {
    // Solo el objeto que abrio la conexion tiene un socket valido (las copias
    // que arma la BD traen -1), asi que no se cierra un socket ajeno.
    if (socketCliente != -1) {
        close(socketCliente);
        socketCliente = -1;
    }
}

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

    string errorCorreo = validarCorreoConNombre(correo, nom, apellidoP, apellidoM, anio);

    if (!errorCorreo.empty()) {
        cout << "No es posible registrar el usuario." << endl;
        cout << errorCorreo << endl;
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

    time_t ahora = time(nullptr);
    if (anio > localtime(&ahora)->tm_year + 1900) {
        return false; // no se puede haber ingresado en el futuro
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

string Cliente::normalizarParaCorreo(const string& texto) {
    string resultado;

    for (size_t i = 0; i < texto.size(); i++) {
        unsigned char c = static_cast<unsigned char>(texto[i]);

        if (c < 0x80) {
            if (isalnum(c)) {
                resultado += static_cast<char>(tolower(c));
            }
            continue;
        }

        // Letras con acento (UTF-8 de 2 bytes: 0xC3 xx). Se pasan a su letra base.
        if (c == 0xC3 && i + 1 < texto.size()) {
            unsigned char d = static_cast<unsigned char>(texto[++i]);
            char base = 0;

            if ((d >= 0x80 && d <= 0x85) || (d >= 0xA0 && d <= 0xA5)) base = 'a';       // Á À Â Ã Ä Å / á à â ã ä å
            else if ((d >= 0x88 && d <= 0x8B) || (d >= 0xA8 && d <= 0xAB)) base = 'e';  // É È Ê Ë
            else if ((d >= 0x8C && d <= 0x8F) || (d >= 0xAC && d <= 0xAF)) base = 'i';  // Í Ì Î Ï
            else if ((d >= 0x92 && d <= 0x96) || (d >= 0xB2 && d <= 0xB6)) base = 'o';  // Ó Ò Ô Õ Ö
            else if ((d >= 0x99 && d <= 0x9C) || (d >= 0xB9 && d <= 0xBC)) base = 'u';  // Ú Ù Û Ü
            else if (d == 0x91 || d == 0xB1) base = 'n';                                // Ñ ñ

            if (base != 0) resultado += base;
        }
    }

    return resultado;
}

string Cliente::validarCorreoConNombre(const string& correo, const string& nombre,
                                       const string& apellidoP, const string& apellidoM, int anio) {
    string nom = normalizarParaCorreo(nombre);
    string ap = normalizarParaCorreo(apellidoP);
    string am = normalizarParaCorreo(apellidoM);

    if (nom.empty() || ap.empty() || am.empty()) {
        return "Nombre y apellidos deben llevar letras.";
    }

    size_t posArroba = correo.find('@');
    string local = normalizarParaCorreo(posArroba == string::npos ? correo : correo.substr(0, posArroba));

    char dosDigitos[8];
    snprintf(dosDigitos, sizeof(dosDigitos), "%02d", anio % 100);

    string prefijo = string(1, nom[0]) + ap + string(1, am[0]) + dosDigitos;

    // El correo debe ser: prefijo (con el anio) + 2 digitos aleatorios
    if (local.size() != prefijo.size() + 2 ||
        !isdigit(static_cast<unsigned char>(local[local.size() - 1])) ||
        !isdigit(static_cast<unsigned char>(local[local.size() - 2]))) {
        return "El correo debe ser " + prefijo + "XX@alumno.ipn.mx (XX = 2 digitos).";
    }

    string inicio = local.substr(0, prefijo.size() - 2);
    string digitosAnio = local.substr(prefijo.size() - 2, 2);

    if (inicio != prefijo.substr(0, prefijo.size() - 2)) {
        return "El correo no coincide con tu nombre: debe ser " + prefijo + "XX@alumno.ipn.mx";
    }

    if (digitosAnio != string(dosDigitos)) {
        return "El anio de ingreso (" + to_string(anio) + ") no coincide con los digitos del correo (" +
               digitosAnio + ").";
    }

    return "";
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


// =====================================================================
//  API para la ventana grafica (todo via el servidor, sin cin/cout)
// =====================================================================

const string& Cliente::getUltimoError() const {
    return ultimoError;
}

bool Cliente::pedirLista(const string& comando, vector<string>& lineas) {
    lineas.clear();

    string respuesta = enviarComando(socketCliente, comando);
    vector<string> encabezado = separarCampos(respuesta);

    if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
        if (!encabezado.empty() && encabezado[0] == "ERR" && encabezado.size() > 1) {
            ultimoError = encabezado[1];
        } else {
            ultimoError = "Respuesta invalida del servidor.";
        }
        return false;
    }

    int cantidad = 0;

    try {
        cantidad = stoi(encabezado[1]);
    } catch (...) {
        ultimoError = "Respuesta invalida del servidor.";
        return false;
    }

    for (int i = 0; i < cantidad; i++) {
        string linea;

        if (!recibirMensaje(socketCliente, linea)) {
            ultimoError = "Se perdio la conexion con el servidor.";
            return false;
        }

        lineas.push_back(linea);
    }

    return true;
}

// El protocolo separa campos con '|' y los productos con ',' y ':', asi que
// esos caracteres no pueden ir dentro de lo que escribe el usuario.
static bool tieneSeparadores(const string& texto) {
    return texto.find('|') != string::npos || texto.find('\n') != string::npos;
}

bool Cliente::iniciarSesion(const string& usuario, const string& clave) {
    if (usuario.empty() || clave.empty()) {
        ultimoError = "Escribe tu usuario y tu contrasena.";
        return false;
    }

    if (tieneSeparadores(usuario) || tieneSeparadores(clave)) {
        ultimoError = "Usuario o contrasena con caracteres no permitidos.";
        return false;
    }

    string respuesta = enviarComando(socketCliente, "LOGIN_CLIENTE|" + usuario + "|" + clave);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        ultimoError = (campos.size() > 1) ? campos[1] : "Error desconocido.";
        return false;
    }

    // OK|nombre|correo|apellidoP|apellidoM|tipoCliente|username
    if (campos.size() < 7) {
        ultimoError = "Respuesta invalida del servidor.";
        return false;
    }

    setNombre(campos[1]);
    setCorreo(campos[2]);
    setApellidoPaterno(campos[3]);
    setApellidoMaterno(campos[4]);
    setTipoCliente(campos[5]);
    setUsername(campos[6]);
    setTipoUsuario("Cliente");

    vaciarCarrito();
    return true;
}

bool Cliente::registrarse(const string& correo, const string& nombre, const string& apellidoP,
                          const string& apellidoM, const string& anio, const string& escuela,
                          const string& contrasena, string& usernameOut) {
    if (correo.empty() || nombre.empty() || apellidoP.empty() || apellidoM.empty() ||
        anio.empty() || escuela.empty() || contrasena.empty()) {
        ultimoError = "Llena todos los campos.";
        return false;
    }

    for (const string* campo : {&correo, &nombre, &apellidoP, &apellidoM, &escuela, &contrasena}) {
        if (tieneSeparadores(*campo)) {
            ultimoError = "No se permite el caracter '|' en los datos.";
            return false;
        }
    }

    int anioNumero = 0;

    try {
        size_t usados = 0;
        anioNumero = stoi(anio, &usados);
        if (usados != anio.size()) throw invalid_argument("anio");
    } catch (...) {
        ultimoError = "El anio de ingreso debe ser un numero (ej. 2023).";
        return false;
    }

    if (!validarIPN(correo, anioNumero, escuela)) {
        ultimoError = "Correo (@alumno.ipn.mx), anio (2015 en adelante) o escuela no validos.";
        return false;
    }

    string errorCorreo = validarCorreoConNombre(correo, nombre, apellidoP, apellidoM, anioNumero);

    if (!errorCorreo.empty()) {
        ultimoError = errorCorreo;
        return false;
    }

    if (contrasena.size() < 8) {
        ultimoError = "La contrasena debe tener minimo 8 caracteres.";
        return false;
    }

    string username = asignarUsername(correo);
    string tipo = (normalizarTexto(escuela) == "UPIITA") ? "UPIITA" : "IPN";

    string comando = "REGISTRO_CLIENTE|" + username + "|" + nombre + "|" + correo + "|" +
                     contrasena + "|" + apellidoP + "|" + apellidoM + "|" + tipo;

    string respuesta = enviarComando(socketCliente, comando);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo registrar.";
        return false;
    }

    usernameOut = username;
    return true;
}

bool Cliente::cargarCafeterias() {
    vector<string> lineas;

    if (!pedirLista("LISTAR_CAFETERIAS", lineas)) {
        return false;
    }

    cafeterias.clear();

    for (const string& linea : lineas) {
        // idCafeteria|nombre|correo|username|pedidos
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 2) {
            continue;
        }

        cafeterias.push_back({campos[0], campos[1]});
    }

    return true;
}

bool Cliente::cargarMenu(const string& idCafeteria) {
    vector<string> lineas;

    if (!pedirLista("INVENTARIO|" + idCafeteria, lineas)) {
        return false;
    }

    menu.clear();

    for (const string& linea : lineas) {
        // idProducto|nombre|stock|precio
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 4) {
            continue;
        }

        Producto p;
        p.setIdProducto(campos[0]);
        p.setNombreProducto(campos[1]);

        try {
            p.setStock(stoi(campos[2]));
            p.setPrecio(stof(campos[3]));
        } catch (...) {
            continue;
        }

        menu.push_back(p);
    }

    return true;
}

const vector<InfoCafeteria>& Cliente::getCafeterias() const {
    return cafeterias;
}

const vector<Producto>& Cliente::getMenu() const {
    return menu;
}

string Cliente::nombreCafeteria(const string& idCafeteria) const {
    for (const auto& c : cafeterias) {
        if (c.id == idCafeteria) {
            return c.nombre;
        }
    }

    return idCafeteria;
}

// ---------------------------- Carrito ----------------------------

const vector<pair<Producto, int>>& Cliente::getCarrito() const {
    return carrito;
}

const string& Cliente::getIdCafeteriaCarrito() const {
    return idCafeteriaCarrito;
}

int Cliente::cantidadEnCarrito(const string& idProducto) const {
    for (const auto& par : carrito) {
        if (par.first.getIdProducto() == idProducto) {
            return par.second;
        }
    }

    return 0;
}

int Cliente::unidadesEnCarrito() const {
    int total = 0;

    for (const auto& par : carrito) {
        total += par.second;
    }

    return total;
}

float Cliente::totalCarrito() const {
    float total = 0.0f;

    for (const auto& par : carrito) {
        total += par.first.getPrecio() * par.second;
    }

    return total;
}

bool Cliente::agregarAlCarrito(const string& idCafeteria, const Producto& producto) {
    // Un pedido es de UNA sola cafeteria: si cambia de cafeteria, se empieza de cero.
    if (!carrito.empty() && idCafeteriaCarrito != idCafeteria) {
        carrito.clear();
    }

    idCafeteriaCarrito = idCafeteria;

    for (auto& par : carrito) {
        if (par.first.getIdProducto() == producto.getIdProducto()) {
            if (par.second + 1 > producto.getStock()) {
                ultimoError = "No hay mas existencias de " + producto.getNombreProducto() + ".";
                return false;
            }

            par.second++;
            return true;
        }
    }

    if (producto.getStock() < 1) {
        ultimoError = producto.getNombreProducto() + " esta agotado.";
        return false;
    }

    carrito.push_back({producto, 1});
    return true;
}

void Cliente::quitarUnoDelCarrito(const string& idProducto) {
    for (size_t i = 0; i < carrito.size(); i++) {
        if (carrito[i].first.getIdProducto() == idProducto) {
            carrito[i].second--;

            if (carrito[i].second <= 0) {
                carrito.erase(carrito.begin() + static_cast<long>(i));
            }

            break;
        }
    }

    if (carrito.empty()) {
        idCafeteriaCarrito.clear();
    }
}

void Cliente::quitarDelCarrito(const string& idProducto) {
    carrito.erase(remove_if(carrito.begin(), carrito.end(),
                            [&](const pair<Producto, int>& par) {
                                return par.first.getIdProducto() == idProducto;
                            }),
                  carrito.end());

    if (carrito.empty()) {
        idCafeteriaCarrito.clear();
    }
}

void Cliente::vaciarCarrito() {
    carrito.clear();
    idCafeteriaCarrito.clear();
}

bool Cliente::crearPedidoDesdeCarrito(string& folioOut, float& totalOut) {
    if (carrito.empty()) {
        ultimoError = "Tu carrito esta vacio.";
        return false;
    }

    if (getUsername().empty()) {
        ultimoError = "Inicia sesion para hacer un pedido.";
        return false;
    }

    // PEDIDO_CREAR|idCafeteria|usernameCliente|idProd1:cant1,idProd2:cant2
    string lista;

    for (const auto& par : carrito) {
        if (!lista.empty()) lista += ",";
        lista += par.first.getIdProducto() + ":" + to_string(par.second);
    }

    string respuesta = enviarComando(socketCliente,
                                     "PEDIDO_CREAR|" + idCafeteriaCarrito + "|" + getUsername() + "|" + lista);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK" || campos.size() < 3) {
        ultimoError = (campos.size() > 1 && campos[0] == "ERR") ? campos[1] : "No se pudo crear el pedido.";
        return false;
    }

    folioOut = campos[1];

    try {
        totalOut = stof(campos[2]);
    } catch (...) {
        totalOut = totalCarrito();
    }

    vaciarCarrito();
    return true;
}

// ------------------------ Pedidos y tarjetas ------------------------

bool Cliente::cargarPedidosCliente() {
    vector<string> lineas;

    if (!pedirLista("PEDIDOS_CLIENTE|" + getUsername(), lineas)) {
        return false;
    }

    pedidosCliente.clear();

    for (const string& linea : lineas) {
        // folio|estado|total|fecha|idCafeteria|pagado
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 6) {
            continue;
        }

        ResumenPedidoCliente r;
        r.folio = campos[0];
        r.estado = campos[1];
        r.fecha = campos[3];
        r.idCafeteria = campos[4];
        r.pagado = (campos[5] == "1");

        try {
            r.total = stof(campos[2]);
        } catch (...) {
            r.total = 0.0f;
        }

        pedidosCliente.push_back(r);
    }

    return true;
}

const vector<ResumenPedidoCliente>& Cliente::getPedidosCliente() const {
    return pedidosCliente;
}

bool Cliente::cargarTarjetasRed() {
    vector<string> lineas;

    if (!pedirLista("TARJETAS_CLIENTE|" + getUsername(), lineas)) {
        return false;
    }

    tarjetasRed.clear();

    for (const string& linea : lineas) {
        // numeroTarjeta|vencimiento
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 2) {
            continue;
        }

        InfoTarjeta t;
        t.numero = campos[0];
        t.vencimiento = campos[1];
        t.enmascarada = Tarjeta(campos[0], "", "", campos[1]).enmascarada();

        tarjetasRed.push_back(t);
    }

    return true;
}

const vector<InfoTarjeta>& Cliente::getTarjetasRed() const {
    return tarjetasRed;
}

bool Cliente::agregarTarjetaRed(const string& numero, const string& cvv, const string& titular,
                                const string& vencimiento) {
    if (tieneSeparadores(numero) || tieneSeparadores(cvv) || tieneSeparadores(titular) ||
        tieneSeparadores(vencimiento)) {
        ultimoError = "No se permite el caracter '|' en los datos.";
        return false;
    }

    // Se valida aqui (rapido, sin ir al servidor) y el servidor lo vuelve a validar.
    if (!Tarjeta::numeroFormatoValido(numero)) {
        ultimoError = "El numero de tarjeta debe tener exactamente 16 digitos.";
        return false;
    }

    if (!Tarjeta::cvvFormatoValido(cvv)) {
        ultimoError = "El CVV debe tener exactamente 3 digitos.";
        return false;
    }

    if (titular.empty()) {
        ultimoError = "Escribe el nombre del titular.";
        return false;
    }

    if (!Tarjeta::fechaFormatoValida(vencimiento)) {
        ultimoError = "Fecha invalida: usa MM/AA (ej. 08/28).";
        return false;
    }

    if (Tarjeta(numero, cvv, titular, vencimiento).estaVencida()) {
        ultimoError = "La tarjeta ya esta vencida.";
        return false;
    }

    string respuesta = enviarComando(socketCliente,
                                     "TARJETA_AGREGAR|" + getUsername() + "|" + numero + "|" + cvv + "|" +
                                     titular + "|" + vencimiento);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo guardar la tarjeta.";
        return false;
    }

    return true;
}

bool Cliente::pagarPedidoRed(const string& folio, const string& numeroTarjeta, const string& cvv,
                             string& referenciaOut) {
    if (!Tarjeta::cvvFormatoValido(cvv)) {
        ultimoError = "El CVV debe tener exactamente 3 digitos.";
        return false;
    }

    string respuesta = enviarComando(socketCliente, "PAGAR_PEDIDO|" + folio + "|" + numeroTarjeta + "|" + cvv);
    vector<string> campos = separarCampos(respuesta);

    // OK|folio|monto|referencia|fecha|tarjetaEnmascarada
    if (campos.empty() || campos[0] != "OK") {
        ultimoError = (campos.size() > 1) ? campos[1] : "No se pudo procesar el pago.";
        return false;
    }

    referenciaOut = (campos.size() > 3) ? campos[3] : "";
    return true;
}
