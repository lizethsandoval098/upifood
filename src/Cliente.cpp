#include "Cliente.h"
#include "Protocolo.h"
#include "Senales.h"

#include <iostream>
#include <iomanip>
#include <map>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include <cctype>

Cliente::Cliente() {
    tipoCliente = "INVITADO";

    socketCliente = -1;
    ipServidor = obtenerIpServidor();
    puerto = obtenerPuertoServidor();
}

Cliente::Cliente(string nombre, string correo, string contrasena, string username,
                  string apellidoPaterno, string apellidoMaterno, string tipoCliente)
    : Usuario("Cliente", nombre, correo, contrasena, username),
      apellidoPaterno{apellidoPaterno}, apellidoMaterno{apellidoMaterno}, tipoCliente{tipoCliente} {

    socketCliente = -1;
    ipServidor = obtenerIpServidor();
    puerto = obtenerPuertoServidor();
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
    std::cout << "Conectando al servidor " << ipServidor << ":" << puerto << "..." << std::endl;

    socketCliente = conectarAlServidor(ipServidor, puerto);

    if (socketCliente == -1) {
        std::cout << "Error conectando al servidor" << std::endl;
        return false;
    }

    std::cout << "Conectado al servidor correctamente" << std::endl;

    return true;
}

bool Cliente::estaConectado() const {
    return socketCliente != -1;
}

void Cliente::entrarComoInvitado(const string& nombreCompleto) {
    setNombre(nombreCompleto);
    setTipoCliente("INVITADO");
    setTipoUsuario("Cliente");
    setUsername("inv_" + to_string(getpid()) + "_" + to_string(time(nullptr) % 100000));
}

void Cliente::cargarTarjetasGuardadas() {
    tarjetasGuardadas.clear();

    if (username.empty() || socketCliente == -1) return;

    vector<string> encabezado = separarCampos(enviarComando(socketCliente, "TARJETAS|" + username));

    if (encabezado.size() < 2 || encabezado[0] != "OK") return;

    int cantidad = 0;
    try { cantidad = stoi(encabezado[1]); } catch (...) { return; }

    for (const string& linea : leerLineas(socketCliente, cantidad)) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 2) continue;

        Tarjeta t;
        t.setNumeroTarjeta(c[0]);
        t.setFechaVencimiento(c[1]);
        t.setNombrePropietario(nombre);
        tarjetasGuardadas.push_back(t);
    }
}

void Cliente::cargarHistorial() {
    historialPedidos.clear();

    if (username.empty() || socketCliente == -1) return;

    // HISTORIAL|user -> OK|N  y luego N lineas: folio|fecha|estado|total|idCafeteria
    vector<string> encabezado = separarCampos(enviarComando(socketCliente, "HISTORIAL|" + username));

    if (encabezado.size() < 2 || encabezado[0] != "OK") return;

    int cantidad = 0;
    try { cantidad = stoi(encabezado[1]); } catch (...) { return; }

    for (const string& linea : leerLineas(socketCliente, cantidad)) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 5) continue;

        Pedido p;
        p.setFolio(c[0]);
        p.setFecha(c[1]);
        p.setEstado(c[2]);
        try { p.setTotal(stof(c[3])); } catch (...) {}
        p.setIdCafeteria(c[4]);
        p.setUsernameCliente(username);
        historialPedidos.push_back(p);
    }
}

void Cliente::cargarPedidoActual() {
    cargarHistorial();

    pedidoActual = Pedido();

    // el mas reciente que todavia no termina
    for (auto it = historialPedidos.rbegin(); it != historialPedidos.rend(); ++it) {
        if (it->getEstado() != "Entregado" && it->getEstado() != "Cancelado") {
            pedidoActual = *it;
            return;
        }
    }
}

void Cliente::verHistorialP() {
    cargarHistorial();

    if (historialPedidos.empty()) {
        cout << "Aun no tienes pedidos registrados." << endl;
        return;
    }

    for (const auto& p : historialPedidos) {
        cout << "Folio: " << p.getFolio()
             << " | Cafeteria: " << p.getIdCafeteria()
             << " | Estado: " << p.getEstado()
             << " | Total: $" << fixed << setprecision(2) << p.getTotal() << endl;
    }
}

void Cliente::hacerPedido() {
    if (socketCliente == -1) {
        cout << "No hay conexion con el servidor." << endl;
        return;
    }

    if (username.empty()) {
        cout << "Inicia sesion o entra como invitado primero." << endl;
        return;
    }

    // Un pedido "Pendiente" puede estar ya pagado (esperando a que la cafeteria
    // lo tome) o sin pagar. Solo se bloquea un pedido nuevo si hay uno SIN pagar.
    cargarPedidoActual();

    if (!pedidoActual.getFolio().empty() && pedidoActual.getEstado() == "Pendiente") {
        vector<string> r = separarCampos(enviarComando(socketCliente, "ESTADO_PEDIDO|" + pedidoActual.getFolio()));

        if (r.size() >= 6 && r[0] == "OK" && r[4] == "0") {
            cout << "Ya tienes el pedido " << pedidoActual.getFolio() << " sin pagar. Pagalo primero (opcion 2)." << endl;
            return;
        }
    }

    int idCaf = 0;
    cout << "Cafeteria (1 o 2): " << flush;
    if (!Senales::leerEntero(idCaf)) return;

    if (idCaf != 1 && idCaf != 2) {
        cout << "Cafeteria invalida." << endl;
        return;
    }

    // INVENTARIO|idCaf -> OK|N y N lineas: id|nombre|stock|precio
    vector<string> encabezado = separarCampos(enviarComando(socketCliente, "INVENTARIO|" + to_string(idCaf)));

    if (encabezado.size() < 2 || encabezado[0] != "OK") {
        cout << "No se pudo cargar el menu." << endl;
        return;
    }

    int cantidadProductos = 0;
    try { cantidadProductos = stoi(encabezado[1]); } catch (...) { return; }

    vector<Producto> menu;

    for (const string& linea : leerLineas(socketCliente, cantidadProductos)) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 4) continue;

        Producto p;
        p.setIdProducto(c[0]);
        p.setNombreProducto(c[1]);
        try {
            p.setStock(stoi(c[2]));
            p.setPrecio(stof(c[3]));
        } catch (...) { continue; }

        menu.push_back(p);
    }

    cout << "\n------ MENU CAFETERIA " << idCaf << " ------" << endl;
    for (const auto& p : menu) {
        cout << "[" << p.getIdProducto() << "] " << left << setw(22) << p.getNombreProducto()
             << " $" << fixed << setprecision(2) << p.getPrecio()
             << "  (quedan " << p.getStock() << ")" << endl;
    }

    Pedido carrito;
    carrito.setUsernameCliente(username);
    carrito.setIdCafeteria(to_string(idCaf));

    while (true) {
        cout << "ID de producto a agregar (ENTER para terminar): " << flush;

        string id;
        if (!Senales::leerLinea(id)) return;
        if (id.empty()) break;

        const Producto* elegido = nullptr;
        for (const auto& p : menu) {
            if (p.getIdProducto() == id) { elegido = &p; break; }
        }

        if (elegido == nullptr) {
            cout << "Ese producto no existe en esta cafeteria." << endl;
            continue;
        }

        cout << "Cantidad: " << flush;
        int cantidad = 0;
        if (!Senales::leerEntero(cantidad)) return;

        if (cantidad < 1 || cantidad > elegido->getStock()) {
            cout << "Cantidad no disponible (stock: " << elegido->getStock() << ")." << endl;
            continue;
        }

        carrito.agregarProducto(*elegido, cantidad);
    }

    if (carrito.getListaProductos().empty()) {
        cout << "Carrito vacio: no se creo ningun pedido." << endl;
        return;
    }

    carrito.generarTotal();
    carrito.mostrarCarrito();

    cout << "Confirmar pedido? (s/n): " << flush;
    string confirmar;
    if (!Senales::leerLinea(confirmar) || (confirmar != "s" && confirmar != "S")) {
        cout << "Pedido cancelado." << endl;
        return;
    }

    string items;
    for (const auto& par : carrito.getListaProductos()) {
        items += (items.empty() ? "" : ";") + par.first.getIdProducto() + ":" + to_string(par.second);
    }

    // CREAR_PEDIDO|username|idCafeteria|id:cant;id:cant -> OK|folio|total
    vector<string> r = separarCampos(enviarComando(socketCliente,
                                     "CREAR_PEDIDO|" + username + "|" + to_string(idCaf) + "|" + items));

    if (r.size() < 3 || r[0] != "OK") {
        cout << (r.size() > 1 ? r[1] : "No se pudo crear el pedido.") << endl;
        return;
    }

    carrito.setFolio(r[1]);
    carrito.setEstado("Pendiente");
    carrito.setFecha("hoy");
    try { carrito.setTotal(stof(r[2])); } catch (...) {}
    pedidoActual = carrito;

    cout << "\nPedido creado. Folio: " << r[1] << "  Total: $" << r[2] << endl;
    cout << "Elige 'Pagar pedido' para que la cafeteria lo empiece a preparar." << endl;
}

void Cliente::pagar() {
    if (socketCliente == -1) {
        cout << "No hay conexion con el servidor." << endl;
        return;
    }

    cargarPedidoActual();

    if (pedidoActual.getFolio().empty()) {
        cout << "No tienes un pedido pendiente de pago." << endl;
        return;
    }

    if (pedidoActual.getEstado() != "Pendiente") {
        cout << "El pedido " << pedidoActual.getFolio() << " ya esta en estado: "
             << pedidoActual.getEstado() << " (no necesita pago)." << endl;
        return;
    }

    cargarTarjetasGuardadas();

    string numero;

    if (!tarjetasGuardadas.empty()) {
        cout << "Tarjeta guardada: " << tarjetasGuardadas[0].getNumeroTarjeta()
             << "  (ENTER para usarla, o escribe otro numero): " << flush;
    } else {
        cout << "Numero de tarjeta (16 digitos): " << flush;
    }

    if (!Senales::leerLinea(numero)) return;

    if (numero.empty() && !tarjetasGuardadas.empty()) {
        numero = tarjetasGuardadas[0].getNumeroTarjeta();
    }

    // quitar espacios/guiones
    string soloDigitos;
    for (char c : numero) if (c >= '0' && c <= '9') soloDigitos += c;

    vector<string> r = separarCampos(enviarComando(socketCliente,
                                     "PAGAR|" + pedidoActual.getFolio() + "|" + soloDigitos));

    if (r.empty() || r[0] != "OK") {
        cout << (r.size() > 1 ? r[1] : "No se pudo pagar.") << endl;
        cargarPedidoActual();
        return;
    }

    pedidoActual.setEstado("Pendiente");
    cout << "Pago aprobado por $" << (r.size() > 3 ? r[3] : "") << "." << endl;
    pedidoActual.generarTicket();
}

void Cliente::recogerPedido() {
    cargarPedidoActual();

    if (pedidoActual.getFolio().empty()) {
        cout << "No tienes un pedido activo." << endl;
        return;
    }

    if (pedidoActual.getEstado() != "Listo") {
        cout << "Tu pedido " << pedidoActual.getFolio() << " esta en estado '"
             << pedidoActual.getEstado() << "'. Aun no esta listo para recoger." << endl;
        return;
    }

    vector<string> r = separarCampos(enviarComando(socketCliente,
                                     "CAMBIAR_ESTADO|" + pedidoActual.getFolio() + "|Entregado"));

    if (r.empty() || r[0] != "OK") {
        cout << (r.size() > 1 ? r[1] : "No se pudo marcar como entregado.") << endl;
        return;
    }

    cout << "Pedido " << pedidoActual.getFolio() << " entregado. Buen provecho!" << endl;
    pedidoActual = Pedido();
}

void Cliente::verEstadoPedido() {
    cargarPedidoActual();

    if (pedidoActual.getFolio().empty()) {
        cout << "No tienes un pedido activo." << endl;
        return;
    }

    // ESTADO_PEDIDO|folio -> OK|folio|estado|total|pagado|idCaf|user
    vector<string> r = separarCampos(enviarComando(socketCliente, "ESTADO_PEDIDO|" + pedidoActual.getFolio()));

    if (r.size() < 6 || r[0] != "OK") {
        cout << "No se pudo consultar el pedido." << endl;
        return;
    }

    pedidoActual.setEstado(r[2]);

    cout << "Pedido #" << r[1] << " (cafeteria " << r[5] << ") -> Estado: " << r[2]
         << " | Pagado: " << (r[4] == "1" ? "si" : "no") << " | Total: $" << r[3] << endl;
}

// ---------------------------------------------------------------------
// Registro / inicio de sesion
// ---------------------------------------------------------------------

bool Cliente::registrarNuevoCliente() {
    string correo, nom, apellidoP, apellidoM, escuela, contrasena;
    int anio = 0;

    cout << "==================================" << endl;
    cout << "Registro de usuario" << endl;

    cout << "Ingrese su correo institucional: " << flush;
    if (!Senales::leerLinea(correo)) return false;

    cout << "Ingrese su nombre SIN APELLIDOS: " << flush;
    if (!Senales::leerLinea(nom)) return false;

    cout << "Ingrese su apellido paterno: " << flush;
    if (!Senales::leerLinea(apellidoP)) return false;

    cout << "Ingrese su apellido materno: " << flush;
    if (!Senales::leerLinea(apellidoM)) return false;

    cout << "Ingrese el anio en el que se unio al IPN: " << flush;
    if (!Senales::leerEntero(anio)) return false;

    cout << "Ingrese el nombre de la escuela de su procedencia: " << flush;
    if (!Senales::leerLinea(escuela)) return false;

    // La validacion del correo/escuela es una regla de negocio pura, no
    // necesita al servidor: se valida aqui mismo antes de gastar una vuelta
    // de red.
    if (!validarIPN(correo, anio, escuela)) {
        cout << "No es posible registrar el usuario." << endl;
        cout << "El correo institucional o la escuela no son validos." << endl;
        return false;
    }

    string username = asignarUsername(correo);

    cout << "Username asignado: " << username << endl;
    cout << "Ingrese su contrasena (minimo 8 caracteres): " << flush;
    if (!Senales::leerLinea(contrasena)) return false;

    if (contrasena.size() < 8) {
        cout << "La contrasena debe tener al menos 8 caracteres." << endl;
        return false;
    }

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

    cout << "Registro realizado correctamente." << endl;
    return true;
}

bool Cliente::iniciarSesionCliente() {
    cout << "==================================" << endl;
    cout << "Inicio de sesion" << endl;

    string username, contrasena;

    cout << "Ingrese username: " << flush;
    if (!Senales::leerLinea(username)) return false;

    cout << "Ingrese contrasena: " << flush;
    if (!Senales::leerLinea(contrasena)) return false;

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
