#include "Servidor.h"
#include "Protocolo.h"
#include "Cliente.h"
#include "Cafeteria.h"
#include "Usuario.h"
#include "Producto.h"
#include "Pedido.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <cstdio>
#include <cctype>
#include <algorithm>

using namespace std;

Servidor::Servidor() {
    socketServidor = -1;
    puerto = 5000;

    for (int i = 0; i < MAX_CAFETERIAS; i++) {
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

    cargarMatrizInventario();

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

    while (true) {
        sockaddr_in direccionCliente;
        socklen_t tam = sizeof(direccionCliente);

        int socketCliente = accept(socketServidor, (sockaddr*)&direccionCliente, &tam);

        if (socketCliente == -1) {
            cout << "Error aceptando una conexion." << endl;
            continue;
        }

        cout << "Nueva conexion aceptada." << endl;

        // Un hilo por cliente: permite atender varias conexiones al mismo tiempo.
        thread hiloCliente(&Servidor::atenderCliente, this, socketCliente);
        hiloCliente.detach();
    }

    return true;
}

void Servidor::atenderCliente(int socketCliente) {
    string usuarioConectado;

    while (true) {
        string comando;

        if (!recibirMensaje(socketCliente, comando)) {
            break;
        }

        cout << "Comando recibido: " << comando << endl;

        string respuesta = procesarComando(comando);

        vector<string> camposComando = separarCampos(comando);
        vector<string> camposRespuesta = separarCampos(respuesta);

        if (!camposComando.empty() && !camposRespuesta.empty() && camposRespuesta[0] == "OK") {
            const string& tipo = camposComando[0];

            if (tipo == "LOGIN_CLIENTE" && camposRespuesta.size() >= 7) {
                usuarioConectado = camposRespuesta[6];
                marcarUsuarioEnLinea(usuarioConectado);
            }
            else if (tipo == "LOGIN_CAFETERIA" && camposRespuesta.size() >= 5) {
                usuarioConectado = camposRespuesta[4];
                marcarUsuarioEnLinea(usuarioConectado);
            }
            else if (tipo == "LOGIN_ADMIN" && camposRespuesta.size() >= 4) {
                usuarioConectado = camposRespuesta[3];
                marcarUsuarioEnLinea(usuarioConectado);
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
    cout << "Conexion cerrada." << endl;
}

void Servidor::cargarMatrizInventario() {
    lock_guard<mutex> guardDb(dbMutex);
    lock_guard<mutex> guardInventario(mutexInventario);

    for (int i = 0; i < MAX_CAFETERIAS; i++) {
        for (int j = 0; j < MAX_PRODUCTOS_CAFETERIA; j++) {
            inventario[i][j] = 0;
        }
    }

    for (int cafeteria = 1; cafeteria <= MAX_CAFETERIAS; cafeteria++) {
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
    // Formato esperado: C1-01, C1-02, ..., C2-01, ... (cafeterias 1 a 9)
    if (idProducto.size() != 5) {
        return false;
    }

    if (idProducto[0] != 'C' || idProducto[2] != '-') {
        return false;
    }

    if (idProducto[1] < '1' || idProducto[1] > '9') {
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

    fila = idProducto[1] - '1';
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

    for (const auto& usuario : usuariosEnLinea) {
        if (usuario == username) {
            return;
        }
    }

    usuariosEnLinea.push_back(username);
}

void Servidor::marcarUsuarioFueraDeLinea(const string& username) {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    for (auto it = usuariosEnLinea.begin(); it != usuariosEnLinea.end(); ++it) {
        if (*it == username) {
            usuariosEnLinea.erase(it);
            return;
        }
    }
}

string Servidor::obtenerUsuariosEnLinea() {
    lock_guard<mutex> guard(mutexUsuariosEnLinea);

    string respuesta = "OK|" + to_string(usuariosEnLinea.size());

    for (const auto& username : usuariosEnLinea) {
        respuesta += "\n" + username;
    }

    return respuesta;
}

// ---------------------------------------------------------------------
// Ayudas para validar productos (agregar / modificar)
// ---------------------------------------------------------------------
static string aMinusculas(string texto) {
    for (char& c : texto) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return texto;
}

static string recortarEspacios(const string& texto) {
    size_t inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == string::npos) return "";
    size_t fin = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fin - inicio + 1);
}

// Regresa "" si todo esta bien; si no, el motivo del error.
static string validarDatosProducto(const string& nombre, const string& textoPrecio,
                                   const string& textoStock, float& precio, int& stock,
                                   int stockMinimo) {
    if (nombre.empty()) return "El nombre del producto no puede estar vacio.";
    if (nombre.size() > 60) return "El nombre es demasiado largo (maximo 60 caracteres).";

    try {
        precio = stof(textoPrecio);
        stock = stoi(textoStock);
    } catch (...) {
        return "Precio o existencias invalidos.";
    }

    if (!(precio > 0.0f) || precio > 99999.0f) return "El precio debe ser mayor a 0.";
    if (stock < stockMinimo || stock > 99999) {
        if (stockMinimo > 0) return "Un producto nuevo debe tener al menos 1 existencia inicial (no puede ser 0).";
        return "Las existencias deben estar entre 0 y 99999.";
    }

    return "";
}

string Servidor::procesarComando(const string& comando) {
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
        return obtenerUsuariosEnLinea();
    }

    // ---------------------------------------------------------------
    // INVENTARIO|idCafeteria
    // ---------------------------------------------------------------
    if (tipo == "INVENTARIO") {
        if (campos.size() < 2) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];

        if (numeroCafeteria(idCafeteria) == 0) {
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

    // ---------------------------------------------------------------
    // RESTOCK|idProducto|cantidad
    // ---------------------------------------------------------------
    if (tipo == "RESTOCK") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string idProducto = campos[1];
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
            return "ERR|ID de producto invalido (formato C1-01).";
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

    // ---------------------------------------------------------------
    // PRODUCTO_AGREGAR|idCafeteria|nombre|precio|stock
    // El ID NO se manda: el servidor lo genera (C1-01, C1-02, ...).
    // -> OK|idProducto|nombre|stock|precio
    // ---------------------------------------------------------------
    if (tipo == "PRODUCTO_AGREGAR") {
        if (campos.size() < 5) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];
        string nombre = recortarEspacios(campos[2]);

        if (numeroCafeteria(idCafeteria) == 0) {
            return "ERR|Cafeteria invalida.";
        }

        float precio = 0.0f;
        int stock = 0;
        string error = validarDatosProducto(nombre, campos[3], campos[4], precio, stock, 1);

        if (!error.empty()) return "ERR|" + error;

        lock_guard<mutex> guardDb(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idCafeteria);

        int mayorNumero = 0;

        for (const auto& producto : productos) {
            if (aMinusculas(producto.getNombreProducto()) == aMinusculas(nombre)) {
                return "ERR|Ya existe un producto llamado \"" + nombre + "\".";
            }

            int fila = 0;
            int columna = 0;

            if (obtenerPosicionProducto(producto.getIdProducto(), fila, columna)) {
                mayorNumero = max(mayorNumero, columna + 1);
            }
        }

        if (mayorNumero >= MAX_PRODUCTOS_CAFETERIA) {
            return "ERR|Esta cafeteria ya tiene el maximo de productos (" +
                   to_string(MAX_PRODUCTOS_CAFETERIA) + ").";
        }

        char idNuevo[16];
        snprintf(idNuevo, sizeof(idNuevo), "C%d-%02d", numeroCafeteria(idCafeteria), mayorNumero + 1);

        Producto nuevo(nombre, idNuevo, stock, precio);

        if (!db.guardarProducto(nuevo)) {
            return "ERR|No se pudo guardar el producto en la base de datos.";
        }

        actualizarMatrizProducto(idNuevo, stock);

        return string("OK|") + idNuevo + "|" + nombre + "|" + to_string(stock) + "|" + to_string(precio);
    }

    // ---------------------------------------------------------------
    // PRODUCTO_MODIFICAR|idCafeteria|idProducto|nombre|precio|stock
    // -> OK|idProducto|nombre|stock|precio
    // ---------------------------------------------------------------
    if (tipo == "PRODUCTO_MODIFICAR") {
        if (campos.size() < 6) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];
        string idProducto = campos[2];
        string nombre = recortarEspacios(campos[3]);

        int fila = 0;
        int columna = 0;

        if (numeroCafeteria(idCafeteria) == 0 ||
            !obtenerPosicionProducto(idProducto, fila, columna) ||
            idProducto[1] - '0' != numeroCafeteria(idCafeteria)) {
            return "ERR|Ese producto no pertenece a esta cafeteria.";
        }

        float precio = 0.0f;
        int stock = 0;
        string error = validarDatosProducto(nombre, campos[4], campos[5], precio, stock, 0);

        if (!error.empty()) return "ERR|" + error;

        lock_guard<mutex> guardDb(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idCafeteria);
        bool existe = false;

        for (const auto& producto : productos) {
            if (producto.getIdProducto() == idProducto) {
                existe = true;
            } else if (aMinusculas(producto.getNombreProducto()) == aMinusculas(nombre)) {
                return "ERR|Ya existe otro producto llamado \"" + nombre + "\".";
            }
        }

        if (!existe) return "ERR|Producto no encontrado.";

        if (!db.actualizarProducto(idProducto, nombre, precio, stock)) {
            return "ERR|No se pudo modificar el producto.";
        }

        actualizarMatrizProducto(idProducto, stock);

        return "OK|" + idProducto + "|" + nombre + "|" + to_string(stock) + "|" + to_string(precio);
    }

    // ---------------------------------------------------------------
    // PRODUCTO_ELIMINAR|idCafeteria|idProducto
    // No deja borrar productos que ya aparecen en pedidos (se perderia
    // el historial); en ese caso se recomienda dejarlo en 0 existencias.
    // -> OK|idProducto
    // ---------------------------------------------------------------
    if (tipo == "PRODUCTO_ELIMINAR") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];
        string idProducto = campos[2];

        int fila = 0;
        int columna = 0;

        if (numeroCafeteria(idCafeteria) == 0 ||
            !obtenerPosicionProducto(idProducto, fila, columna) ||
            idProducto[1] - '0' != numeroCafeteria(idCafeteria)) {
            return "ERR|Ese producto no pertenece a esta cafeteria.";
        }

        lock_guard<mutex> guardDb(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idCafeteria);
        bool existe = false;

        for (const auto& producto : productos) {
            if (producto.getIdProducto() == idProducto) {
                existe = true;
                break;
            }
        }

        if (!existe) return "ERR|Producto no encontrado.";

        if (db.productoTienePedidos(idProducto)) {
            return "ERR|No se puede eliminar: ya aparece en pedidos. Ponle 0 existencias para dejar de venderlo.";
        }

        if (!db.eliminarProducto(idProducto)) {
            return "ERR|No se pudo eliminar el producto.";
        }

        actualizarMatrizProducto(idProducto, 0);

        return "OK|" + idProducto;
    }

    // ---------------------------------------------------------------
    // PEDIDOS_CAFETERIA|idCafeteria
    // -> OK|N  y luego N lineas:  folio|username|estado|total|fecha
    // (los mas recientes primero)
    // ---------------------------------------------------------------
    if (tipo == "PEDIDOS_CAFETERIA") {
        if (campos.size() < 2) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];

        if (idCafeteria.empty()) {
            return "ERR|Cafeteria invalida.";
        }

        lock_guard<mutex> guard(dbMutex);

        vector<Pedido> pedidos = db.obtenerPedidosCafeteria(idCafeteria);
        string respuesta = "OK|" + to_string(pedidos.size());

        for (const auto& pedido : pedidos) {
            respuesta += "\n" + pedido.getFolio() + "|" +
                         pedido.getUsernameCliente() + "|" +
                         pedido.getEstado() + "|" +
                         to_string(pedido.getTotal()) + "|" +
                         pedido.getFecha();
        }

        return respuesta;
    }

    // ---------------------------------------------------------------
    // PEDIDO_DETALLE|idCafeteria|folio
    // -> OK|N  y luego N lineas:  idProducto|nombre|cantidad|precioUnitario
    // ---------------------------------------------------------------
    if (tipo == "PEDIDO_DETALLE") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];
        string folio = campos[2];

        lock_guard<mutex> guard(dbMutex);

        Pedido pedido = db.obtenerPedido_Folio(folio);

        if (pedido.getFolio().empty()) {
            return "ERR|Pedido no encontrado.";
        }

        if (pedido.getIdCafeteria() != idCafeteria) {
            return "ERR|Ese pedido no pertenece a esta cafeteria.";
        }

        vector<pair<Producto, int>> lista = pedido.getListaProductos();
        string respuesta = "OK|" + to_string(lista.size());

        for (const auto& par : lista) {
            respuesta += "\n" + par.first.getIdProducto() + "|" +
                         par.first.getNombreProducto() + "|" +
                         to_string(par.second) + "|" +
                         to_string(par.first.getPrecio());
        }

        return respuesta;
    }

    // ---------------------------------------------------------------
    // CAMBIAR_ESTADO|idCafeteria|folio|nuevoEstado
    // Flujo permitido:  Pendiente -> Preparando -> Listo -> Entregado
    //                   (y se puede Cancelar desde Pendiente o Preparando)
    // ---------------------------------------------------------------
    if (tipo == "CAMBIAR_ESTADO") {
        if (campos.size() < 4) return "ERR|Formato invalido.";

        string idCafeteria = campos[1];
        string folio = campos[2];
        string nuevoEstado = campos[3];

        lock_guard<mutex> guard(dbMutex);

        Pedido pedido = db.obtenerPedido_Folio(folio);

        if (pedido.getFolio().empty()) {
            return "ERR|Pedido no encontrado.";
        }

        if (pedido.getIdCafeteria() != idCafeteria) {
            return "ERR|Ese pedido no pertenece a esta cafeteria.";
        }

        string actual = pedido.getEstado();

        bool permitido =
            (actual == "Pendiente"  && (nuevoEstado == "Preparando" || nuevoEstado == "Cancelado")) ||
            (actual == "Preparando" && (nuevoEstado == "Listo"      || nuevoEstado == "Cancelado")) ||
            (actual == "Listo"      &&  nuevoEstado == "Entregado");

        if (!permitido) {
            return "ERR|No se puede pasar el pedido de " + actual + " a " + nuevoEstado + ".";
        }

        if (!db.actualizarEstadoPedido(folio, nuevoEstado)) {
            return "ERR|No se pudo actualizar el pedido.";
        }

        return "OK|" + folio + "|" + nuevoEstado;
    }

    // ---------------------------------------------------------------
    // VENTA_CAJA|idProducto|cantidad
    // Venta directa en caja: descuenta del inventario.
    // -> OK|idProducto|nuevoStock|precioUnitario
    // ---------------------------------------------------------------
    if (tipo == "VENTA_CAJA") {
        if (campos.size() < 3) return "ERR|Formato invalido.";

        string idProducto = campos[1];
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
            return "ERR|ID de producto invalido (formato C1-01).";
        }

        lock_guard<mutex> guardDb(dbMutex);

        vector<Producto> productos = db.obtenerInventario(idProducto.substr(1, 1));
        int stockActual = -1;
        float precio = 0.0f;

        for (const auto& producto : productos) {
            if (producto.getIdProducto() == idProducto) {
                stockActual = producto.getStock();
                precio = producto.getPrecio();
                break;
            }
        }

        if (stockActual < 0) {
            return "ERR|Producto no encontrado.";
        }

        if (stockActual < cantidad) {
            return "ERR|Stock insuficiente (quedan " + to_string(stockActual) + ").";
        }

        int nuevoStock = stockActual - cantidad;

        if (!db.actualizarExistencia(idProducto, nuevoStock)) {
            return "ERR|No se pudo actualizar el inventario.";
        }

        actualizarMatrizProducto(idProducto, nuevoStock);

        return "OK|" + idProducto + "|" + to_string(nuevoStock) + "|" + to_string(precio);
    }

    return "ERR|Comando desconocido: " + tipo;
}
