#include "Administrador.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

using namespace std;

Administrador::Administrador() {
    socketAdmin = -1;
    ipServidor = "100.70.231.3"; // 100.91.99.27
    ipServidor = "100.70.231.3";
    puerto = 5000;
}

Administrador::Administrador(string nombre, string correo, string contrasena, string username)
    : Usuario("Admin", nombre, correo, contrasena, username) {
    socketAdmin = -1;
    ipServidor = "100.70.231.3";
    puerto = 5000;
}

Administrador::~Administrador() {
    if (socketAdmin != -1) {
        close(socketAdmin);
        socketAdmin = -1;
    }
}

void Administrador::setIpServidor(const string& ip) {
    ipServidor = ip;
}

bool Administrador::conectar() {
    socketAdmin = socket(AF_INET, SOCK_STREAM, 0);

    if (socketAdmin == -1) {
        ultimoError = "Error creando el socket.";
        cout << ultimoError << endl;
        return false;
    }

    sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(puerto);

    if (inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr) != 1) {
        ultimoError = "La IP del servidor no es valida: " + ipServidor;
        cout << ultimoError << endl;
        close(socketAdmin);
        socketAdmin = -1;
        return false;
    }

    if (connect(socketAdmin, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
        ultimoError = "No se pudo conectar a " + ipServidor + ". Revisa Tailscale y que 'servidor' este corriendo.";
        cout << ultimoError << endl;
        close(socketAdmin);
        socketAdmin = -1;
        return false;
    }

    // Si el servidor no contesta en 5 s, recv() falla en vez de quedarse
    // esperando para siempre (y congelar la ventana).
    timeval limite;
    limite.tv_sec = 5;
    limite.tv_usec = 0;
    setsockopt(socketAdmin, SOL_SOCKET, SO_RCVTIMEO, &limite, sizeof(limite));

    cout << "Conectado al servidor (" << ipServidor << ")." << endl;
    return true;
}

const vector<Usuario>& Administrador::getListaUsuarios() const {
    return listaUsuarios;
}

const vector<Cafeteria>& Administrador::getListaCafeterias() const {
    return listaCafeterias;
}

const vector<ResumenCafeteria>& Administrador::getResumenCafeterias() const {
    return resumenCafeterias;
}

const vector<string>& Administrador::getUsuariosEnLinea() const {
    return usuariosEnLinea;
}

string Administrador::getUltimoError() const {
    return ultimoError;
}

// ---------------------------------------------------------------------
// Peticiones de lista: el servidor contesta "OK|N" y luego N lineas mas.
// ---------------------------------------------------------------------
bool Administrador::pedirLista(const string& comando, vector<string>& lineas) {
    lineas.clear();

    string respuesta = enviarComando(socketAdmin, comando);
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

        if (!recibirMensaje(socketAdmin, linea)) {
            ultimoError = "Se perdio la conexion con el servidor.";
            return false;
        }

        lineas.push_back(linea);
    }

    return true;
}

bool Administrador::cargarUsuarios() {
    vector<string> lineas;

    if (!pedirLista("LISTAR_USUARIOS", lineas)) {
        return false;
    }

    listaUsuarios.clear();

    for (const string& linea : lineas) {
        // nombre|username|correo|tipoUsuario
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 4) {
            continue;
        }

        Usuario usuario;
        usuario.setNombre(campos[0]);
        usuario.setUsername(campos[1]);
        usuario.setCorreo(campos[2]);
        usuario.setTipoUsuario(campos[3]);

        listaUsuarios.push_back(usuario);
    }

    return true;
}

bool Administrador::cargarCafeterias() {
    vector<string> lineas;

    if (!pedirLista("LISTAR_CAFETERIAS", lineas)) {
        return false;
    }

    listaCafeterias.clear();
    resumenCafeterias.clear();

    for (const string& linea : lineas) {
        // idCafeteria|nombre|correo|username|cantidadPedidos
        vector<string> campos = separarCampos(linea);

        if (campos.size() < 5) {
            continue;
        }

        ResumenCafeteria r;
        r.idCafeteria = campos[0];
        r.nombre = campos[1];
        r.correo = campos[2];
        r.username = campos[3];

        try {
            r.pedidos = stoi(campos[4]);
        } catch (...) {
            r.pedidos = 0;
        }

        resumenCafeterias.push_back(r);
        listaCafeterias.push_back(Cafeteria(r.nombre, r.correo, "", r.username, r.idCafeteria));
    }

    return true;
}

bool Administrador::cargarUsuariosEnLinea() {
    vector<string> lineas;

    if (!pedirLista("LISTAR_USUARIOS_EN_LINEA", lineas)) {
        return false;
    }

    usuariosEnLinea = lineas;
    return true;
}

// ---------------------------------------------------------------------
// Versiones de consola
// ---------------------------------------------------------------------
void Administrador::verUsuarios() {
    if (!cargarUsuarios()) {
        cout << "No se pudo cargar la lista de usuarios: " << ultimoError << endl;
        return;
    }

    if (listaUsuarios.empty()) {
        cout << "No hay usuarios registrados." << endl;
        return;
    }

    for (size_t i = 0; i < listaUsuarios.size(); i++) {
        cout << "Nombre: " << listaUsuarios[i].getNombre() << endl;
        cout << "Username: " << listaUsuarios[i].getUsername() << endl;
        cout << "Correo: " << listaUsuarios[i].getCorreo() << endl;
        cout << "Tipo de usuario: " << listaUsuarios[i].getTipoUsuario() << endl;
        cout << "----------------------------------" << endl;
    }
}

void Administrador::verCafeterias() {
    if (!cargarCafeterias()) {
        cout << "No se pudo cargar la lista de cafeterias: " << ultimoError << endl;
        return;
    }

    if (resumenCafeterias.empty()) {
        cout << "No hay cafeterias registradas." << endl;
        return;
    }

    for (const auto& r : resumenCafeterias) {
        cout << "Cafeteria [" << r.idCafeteria << "] " << r.nombre
             << " -> " << r.pedidos << " pedidos" << endl;
    }
}

void Administrador::verUsuariosEnLinea() {
    if (!cargarUsuariosEnLinea()) {
        cout << "No se pudo obtener la lista de usuarios en linea: " << ultimoError << endl;
        return;
    }

    if (usuariosEnLinea.empty()) {
        cout << "No hay usuarios conectados en este momento." << endl;
        return;
    }

    cout << "Usuarios en linea (" << usuariosEnLinea.size() << "):" << endl;

    for (const auto& username : usuariosEnLinea) {
        cout << " - " << username << endl;
    }
}

// ---------------------------------------------------------------------
// Login
// ---------------------------------------------------------------------
bool Administrador::iniciarSesionAdmin(const string& username, const string& contrasena) {
    if (username.empty() || contrasena.empty()) {
        ultimoError = "Escribe tu usuario y tu contrasena.";
        return false;
    }

    string respuesta = enviarComando(socketAdmin, "LOGIN_ADMIN|" + username + "|" + contrasena);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        ultimoError = (campos.size() > 1) ? campos[1] : "Error desconocido.";
        return false;
    }

    // Respuesta esperada: OK|nombre|correo|username
    if (campos.size() < 4) {
        ultimoError = "Respuesta invalida del servidor.";
        return false;
    }

    setNombre(campos[1]);
    setCorreo(campos[2]);
    setUsername(campos[3]);
    setTipoUsuario("Admin");

    ultimoError.clear();
    return true;
}

bool Administrador::iniciarSesionAdmin() {
    cout << "==================================" << endl;
    cout << "Inicio de sesion (Administrador)" << endl;

    string username, contrasena;

    cout << "Ingrese username: ";
    cin >> username;

    cout << "Ingrese contrasena: ";
    cin >> contrasena;

    if (!iniciarSesionAdmin(username, contrasena)) {
        cout << ultimoError << endl;
        return false;
    }

    cout << "Inicio de sesion correcto." << endl;
    return true;
}
