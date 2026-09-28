#include "Administrador.h"
#include "Protocolo.h"
#include "Senales.h"

#include <iomanip>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

Administrador::Administrador() {
    socketAdmin = -1;
    ipServidor = obtenerIpServidor();
    puerto = obtenerPuertoServidor();
}

Administrador::Administrador(string nombre, string correo, string contrasena, string username)
    : Usuario("Admin", nombre, correo, contrasena, username) {
    socketAdmin = -1;
    ipServidor = obtenerIpServidor();
    puerto = obtenerPuertoServidor();
}

Administrador::~Administrador() {
    if (socketAdmin != -1) {
        close(socketAdmin);
        socketAdmin = -1;
    }
}

bool Administrador::conectar() {
    socketAdmin = conectarAlServidor(ipServidor, puerto);

    if (socketAdmin == -1) {
        cout << "Error conectando al servidor " << ipServidor << ":" << puerto << ". Revisa la IP/Tailscale." << endl;
        return false;
    }

    cout << "Conectado al servidor correctamente." << endl;
    return true;
}

const vector<Usuario>& Administrador::getListaUsuarios() const {
    return listaUsuarios;
}

const vector<Cafeteria>& Administrador::getListaCafeterias() const {
    return listaCafeterias;
}

void Administrador::cargarUsuarios() {
    string respuesta = enviarComando(socketAdmin, "LISTAR_USUARIOS");
    vector<string> encabezado = separarCampos(respuesta);

    listaUsuarios.clear();

    if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
        cout << "No se pudo cargar la lista de usuarios." << endl;
        if (encabezado.size() > 1) cout << encabezado[1] << endl;
        return;
    }

    int cantidad = 0;
    try {
        cantidad = stoi(encabezado[1]);
    } catch (...) {
        cout << "Respuesta invalida del servidor." << endl;
        return;
    }

    for (int i = 0; i < cantidad; i++) {
        string linea;
        if (!recibirMensaje(socketAdmin, linea)) {
            cout << "No se recibieron todos los usuarios." << endl;
            return;
        }

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
}

void Administrador::verUsuarios() {
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
    string respuesta = enviarComando(socketAdmin, "LISTAR_CAFETERIAS");
    vector<string> encabezado = separarCampos(respuesta);

    listaCafeterias.clear();

    if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
        cout << "No se pudo cargar la lista de cafeterias." << endl;
        if (encabezado.size() > 1) cout << encabezado[1] << endl;
        return;
    }

    int cantidad = 0;
    try {
        cantidad = stoi(encabezado[1]);
    } catch (...) {
        cout << "Respuesta invalida del servidor." << endl;
        return;
    }

    for (int i = 0; i < cantidad; i++) {
        string linea;
        if (!recibirMensaje(socketAdmin, linea)) {
            cout << "No se recibieron todas las cafeterias." << endl;
            return;
        }

        vector<string> campos = separarCampos(linea);

        if (campos.size() < 5) {
            continue;
        }

        Cafeteria cafeteria(campos[1], campos[2], "", campos[3], campos[0]);
        listaCafeterias.push_back(cafeteria);

        cout << "Cafeteria [" << campos[0] << "] " << campos[1]
             << " -> " << campos[4] << " pedidos" << endl;
    }
}

void Administrador::verUsuariosEnLinea() {
    string respuesta = enviarComando(socketAdmin, "LISTAR_USUARIOS_EN_LINEA");
    vector<string> encabezado = separarCampos(respuesta);

    if (encabezado.empty() || encabezado[0] != "OK" || encabezado.size() < 2) {
        cout << "No se pudo obtener la lista de usuarios en linea." << endl;
        if (encabezado.size() > 1) cout << encabezado[1] << endl;
        return;
    }

    int cantidad = 0;
    try {
        cantidad = stoi(encabezado[1]);
    } catch (...) {
        cout << "Respuesta invalida del servidor." << endl;
        return;
    }

    if (cantidad == 0) {
        cout << "No hay usuarios conectados en este momento." << endl;
        return;
    }

    cout << "Usuarios en linea (" << cantidad << "):" << endl;

    for (int i = 0; i < cantidad; i++) {
        string username;
        if (!recibirMensaje(socketAdmin, username)) {
            cout << "No se recibieron todos los usuarios en linea." << endl;
            return;
        }

        cout << " - " << username << endl;
    }
}

void Administrador::verEstadisticas() {
    // ESTADISTICAS -> OK|2 y 2 lineas: idCaf|stockTotal|unidadesVendidas|pend|prep|listo|entr|canc
    vector<string> encabezado = separarCampos(enviarComando(socketAdmin, "ESTADISTICAS"));

    if (encabezado.size() < 2 || encabezado[0] != "OK") {
        cout << "No se pudieron obtener las estadisticas." << endl;
        return;
    }

    int cantidad = 0;
    try { cantidad = stoi(encabezado[1]); } catch (...) { return; }

    cout << "\n" << left << setw(11) << "Cafeteria" << setw(8) << "Stock" << setw(10) << "Vendidas"
         << setw(11) << "Pendiente" << setw(12) << "Preparando" << setw(7) << "Listo"
         << setw(11) << "Entregado" << "Cancelado" << endl;

    for (const string& linea : leerLineas(socketAdmin, cantidad)) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 8) continue;

        cout << left << setw(11) << c[0] << setw(8) << c[1] << setw(10) << c[2]
             << setw(11) << c[3] << setw(12) << c[4] << setw(7) << c[5]
             << setw(11) << c[6] << c[7] << endl;
    }
}

bool Administrador::iniciarSesionAdmin() {
    cout << "==================================" << endl;
    cout << "Inicio de sesion (Administrador)" << endl;

    string username, contrasena;

    cout << "Ingrese username: " << flush;
    if (!Senales::leerLinea(username)) return false;

    cout << "Ingrese contrasena: " << flush;
    if (!Senales::leerLinea(contrasena)) return false;

    string respuesta = enviarComando(socketAdmin, "LOGIN_ADMIN|" + username + "|" + contrasena);
    vector<string> campos = separarCampos(respuesta);

    if (campos.empty() || campos[0] != "OK") {
        string motivo = (campos.size() > 1) ? campos[1] : "Error desconocido.";
        cout << motivo << endl;
        return false;
    }

    if (campos.size() < 4) {
        cout << "Respuesta invalida del servidor." << endl;
        return false;
    }

    setNombre(campos[1]);
    setCorreo(campos[2]);
    setUsername(campos[3]);
    setTipoUsuario("Admin");

    cout << "Inicio de sesion correcto." << endl;
    return true;
}
