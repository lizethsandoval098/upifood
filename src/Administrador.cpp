#include "Administrador.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <utility>

using namespace std;

Administrador::Administrador() {
    socketAdmin = -1;
    ipServidor = "100.91.99.27";
    puerto = 5000;
}

Administrador::Administrador(string nombre, string correo, string contrasena, string username)
    : Usuario("Admin", nombre, correo, contrasena, username) {
    socketAdmin = -1;
    ipServidor = "100.91.99.27";
    puerto = 5000;
}

Administrador::~Administrador() {
    if (socketAdmin != -1) {
        close(socketAdmin);
        socketAdmin = -1;
    }
}

bool Administrador::conectar() {
    socketAdmin = socket(AF_INET, SOCK_STREAM, 0);

    if (socketAdmin == -1) {
        cout << "Error creando socket." << endl;
        return false;
    }

    sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(puerto);
    inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr);

    if (connect(socketAdmin, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
        cout << "Error conectando al servidor. Revisa la IP/Tailscale." << endl;
        close(socketAdmin);
        socketAdmin = -1;
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

vector<Pedido> Administrador::getListaPedidos() const {
    lock_guard<mutex> guard(mutexPedidos);
    return listaPedidos;
}

void Administrador::cargarPedidos() {
    vector<Pedido> pedidosActualizados;
    {
        lock_guard<mutex> guardSocket(mutexSocket);
        string respuesta = enviarComando(socketAdmin, "LISTAR_PEDIDOS_ADMIN");
        vector<string> encabezado = separarCampos(respuesta);
        if (encabezado.size() < 2 || encabezado[0] != "OK") return;

        int cantidad = 0;
        try {
            cantidad = stoi(encabezado[1]);
        } catch (...) {
            return;
        }

        for (int i = 0; i < cantidad; ++i) {
            string linea;
            if (!recibirMensaje(socketAdmin, linea)) return;
            vector<string> campos = separarCampos(linea);
            if (campos.size() < 6) continue;

            Pedido pedido;
            pedido.setFolio(campos[0]);
            pedido.setFecha(campos[1]);
            pedido.setEstado(campos[2]);
            try {
                pedido.setTotal(stof(campos[3]));
            } catch (...) {
                continue;
            }
            pedido.setUsernameCliente(campos[4]);
            pedido.setIdCafeteria(campos[5]);
            pedidosActualizados.push_back(pedido);
        }
    }

    lock_guard<mutex> guardPedidos(mutexPedidos);
    listaPedidos = move(pedidosActualizados);
}

void Administrador::verPedidos() const {
    vector<Pedido> pedidos = getListaPedidos();
    if (pedidos.empty()) {
        cout << "No hay pedidos registrados." << endl;
        return;
    }

    for (const Pedido& pedido : pedidos) {
        cout << "Folio " << pedido.getFolio() << " | Cafe " << pedido.getIdCafeteria()
             << " | Cliente " << pedido.getUsernameCliente() << " | " << pedido.getEstado()
             << " | $" << pedido.getTotal() << " | " << pedido.getFecha() << endl;
    }
}

void Administrador::cargarUsuarios() {
    lock_guard<mutex> guardSocket(mutexSocket);
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
    lock_guard<mutex> guardSocket(mutexSocket);
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
    lock_guard<mutex> guardSocket(mutexSocket);
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

bool Administrador::iniciarSesionAdmin() {
    cout << "==================================" << endl;
    cout << "Inicio de sesion (Administrador)" << endl;

    string username, contrasena;

    cout << "Ingrese username: ";
    cin >> username;

    cout << "Ingrese contrasena: ";
    cin >> contrasena;

    string respuesta;
    {
        lock_guard<mutex> guardSocket(mutexSocket);
        respuesta = enviarComando(socketAdmin, "LOGIN_ADMIN|" + username + "|" + contrasena);
    }
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
