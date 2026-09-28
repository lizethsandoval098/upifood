// =====================================================================
//  UPIIFOOD - Cafeteria (interfaz grafica con SFML)
//
//  Misma idea que mainAdministrador.cpp:
//
//    La VENTANA ES la interfaz. Todo se hace con clics y teclado dentro
//    de la ventana, en UN SOLO CICLO que se repite ~60 veces por segundo:
//
//         mientras la ventana este abierta:
//             1) atender eventos  (clics, teclas, cerrar)  <- pollEvent
//             2) actualizar datos (pedirle cosas al servidor cuando toque)
//             3) dibujar todo     (clear -> draw -> display)
//
//    NUNCA hay que quedarse bloqueado esperando (cin, sleep largos...)
//    con la ventana abierta, porque entonces no se atienden eventos y
//    el sistema dice "no responde -> Forzar salida".
//
//  Pantallas: LOGIN (usuario/contrasena) -> PANEL (botones + tablas)
//  Vistas del PANEL:
//     PEDIDOS    - pedidos que hicieron los clientes; se avanzan de estado
//                  (Pendiente -> Preparando -> Listo -> Entregado), se llevan
//                  de golpe hasta Entregado, o se cancelan.
//     INVENTARIO - agregar / modificar / eliminar productos y reabastecer.
//                  Todo se guarda en la base de datos (via servidor). Los
//                  productos NUEVOS se juntan en una lista y un boton los
//                  sube todos; el ID se genera solo y no se puede editar.
//     CAJA       - venta directa en mostrador (descuenta del inventario).
//
//  La cafeteria NUNCA abre la base de datos: todo pasa por el servidor.
// =====================================================================

#include "Cafeteria.h"
#include "Ventana.h"
#include "Widgets.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cctype>

using namespace std;

enum class Pantalla { LOGIN, PANEL };
enum class Vista { PEDIDOS, INVENTARIO, CAJA };

// ---- Colores (los mismos del administrador) ----
static const sf::Color FONDO(245, 241, 233);
static const sf::Color BARRA_LATERAL(74, 50, 35);
static const sf::Color CREMA(250, 246, 238);
static const sf::Color TEXTO_OSCURO(50, 40, 35);
static const sf::Color TEXTO_SUAVE(130, 120, 110);
static const sf::Color ROJO_ERROR(180, 40, 40);
static const sf::Color VERDE(40, 160, 90);
static const sf::Color AMBAR(200, 130, 20);
static const sf::Color AZUL(50, 110, 180);
static const sf::Color FILA_CLARA(255, 255, 255);
static const sf::Color FILA_OSCURA(246, 240, 230);
static const sf::Color FILA_SELECCION(232, 208, 176);

// ---- Medidas de la ventana y de la zona de tablas ----
static const int ANCHO = 1100;
static const int ALTO = 720;
static const float TABLA_X = 250.0f;          // donde empiezan las tablas
static const float TABLA_ANCHO = 830.0f;      // ancho de las tablas (llega hasta x = 1080)
static const float TABLA_Y_FILAS = 140.0f;
static const float ALTO_FILA = 30.0f;
static const int FILAS_INVENTARIO = 8;
static const int FILAS_CAJA = 9;
static const int FILAS_PEDIDOS = 8;
static const float SEGUNDOS_REFRESCO = 3.0f;
static const float SEGUNDOS_MENSAJE = 5.0f;
static const float SEGUNDOS_CONFIRMAR = 4.0f;
static const sf::Color FILA_NUEVA(255, 243, 210);

// Acorta un texto largo para que no se salga de su columna
// (cuidando no partir a la mitad una letra con acento).
static string recortar(const string& s, size_t maximo) {
    if (s.size() <= maximo) return s;

    size_t corte = maximo;
    while (corte > 0 && (static_cast<unsigned char>(s[corte]) & 0xC0) == 0x80) {
        corte--;
    }

    return s.substr(0, corte) + "...";
}

// 12.5 -> "$12.50"
static string dinero(float cantidad) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "$%.2f", cantidad);
    return buffer;
}

// Convierte el texto del campo en una cantidad valida (1 a 9999).
// Regresa -1 si esta vacio o no sirve.
static int leerCantidad(const string& texto) {
    if (texto.empty() || texto.size() > 4) return -1;

    int valor = 0;
    for (char c : texto) {
        if (c < '0' || c > '9') return -1;
        valor = valor * 10 + (c - '0');
    }

    return (valor >= 1) ? valor : -1;
}

// Cantidad de existencias: 0 a 99999. Regresa -1 si no sirve.
static int leerExistencias(const string& texto) {
    if (texto.empty() || texto.size() > 5) return -1;

    int valor = 0;
    for (char c : texto) {
        if (c < '0' || c > '9') return -1;
        valor = valor * 10 + (c - '0');
    }

    return valor;
}

// Precio: "12", "12.5" o "12.50". Regresa -1 si no sirve (o si es 0).
static float leerPrecio(const string& texto) {
    if (texto.empty() || texto.size() > 8) return -1.0f;

    bool hayPunto = false;
    bool hayDigito = false;

    for (char c : texto) {
        if (c == '.') {
            if (hayPunto) return -1.0f;
            hayPunto = true;
        } else if (c >= '0' && c <= '9') {
            hayDigito = true;
        } else {
            return -1.0f;
        }
    }

    if (!hayDigito) return -1.0f;

    float valor = 0.0f;
    try {
        valor = stof(texto);
    } catch (...) {
        return -1.0f;
    }

    return (valor > 0.0f) ? valor : -1.0f;
}

// 12.5 -> "12.50" (sin el signo $, para llenar el campo de precio)
static string precioTexto(float precio) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", precio);
    return buffer;
}

static string minusculas(string texto) {
    for (char& c : texto) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return texto;
}

// Dibuja un rectangulo de color (fondos, franjas, lineas).
static void rectangulo(Ventana& v, float x, float y, float ancho, float alto, sf::Color color) {
    sf::RectangleShape r(sf::Vector2f(ancho, alto));
    r.setPosition(x, y);
    r.setFillColor(color);
    v.getWindow().draw(r);
}

// Dibuja los titulos de las columnas y la linea de abajo.
static void encabezadoTabla(Ventana& v, const string& titulo, const string& subtitulo,
                            const vector<pair<string, float>>& columnas) {
    v.dibujarTexto(titulo, 260, 30, 26, TEXTO_OSCURO);
    v.dibujarTexto(subtitulo, 260, 66, 15, TEXTO_SUAVE);

    for (const auto& c : columnas) {
        v.dibujarTexto(c.first, c.second, 104, 16, TEXTO_SUAVE);
    }

    rectangulo(v, TABLA_X, 130, TABLA_ANCHO, 2, sf::Color(200, 190, 178));
}

// Franja de fondo de la fila i (alternando color para que se lea mejor;
// la fila seleccionada se pinta de otro color).
static void franja(Ventana& v, int i, bool seleccionada = false, bool nueva = false) {
    float y = TABLA_Y_FILAS + i * ALTO_FILA;
    sf::Color color = seleccionada ? FILA_SELECCION
                    : nueva        ? FILA_NUEVA
                                   : ((i % 2 == 0) ? FILA_CLARA : FILA_OSCURA);
    rectangulo(v, TABLA_X, y - 3, TABLA_ANCHO, ALTO_FILA - 2, color);
}

static sf::Color colorEstado(const string& estado) {
    if (estado == "Pendiente")  return AMBAR;
    if (estado == "Preparando") return AZUL;
    if (estado == "Listo")      return VERDE;
    if (estado == "Entregado")  return TEXTO_SUAVE;
    return ROJO_ERROR;          // Cancelado
}

static bool estaActivo(const string& estado) {
    return estado == "Pendiente" || estado == "Preparando" || estado == "Listo";
}

// Cual es el siguiente paso normal de un pedido (y como se llama el boton).
static bool siguienteEstado(const string& estado, string& textoBoton, string& nuevoEstado) {
    if (estado == "Pendiente")  { textoBoton = "Empezar a preparar"; nuevoEstado = "Preparando"; return true; }
    if (estado == "Preparando") { textoBoton = "Marcar como listo";  nuevoEstado = "Listo";      return true; }
    if (estado == "Listo")      { textoBoton = "Entregar pedido";    nuevoEstado = "Entregado";  return true; }
    return false;
}

static bool sePuedeCancelar(const string& estado) {
    return estado == "Pendiente" || estado == "Preparando";
}

int main(int argc, char* argv[]) {
    string ip = resolverIpServidor(argc, argv);

    Cafeteria cafeteria;
    cafeteria.setIpServidor(ip);

    // 1) Conectar al servidor ANTES de abrir la ventana (si falla, avisamos por consola).
    if (!cafeteria.conectar()) {
        return 1;
    }

    // 2) Ahora si: abrir la ventana. Desde aqui el programa SOLO vive dentro del ciclo.
    Ventana ventana("UPIIFOOD - Cafetería", ANCHO, ALTO);
    ventana.abrir();
    sf::RenderWindow& win = ventana.getWindow();

    // ---------------- Componentes de la pantalla LOGIN ----------------
    const float loginX = (ANCHO - 300) / 2.0f;
    CampoTexto campoUsuario("Usuario", loginX, 215, 300, 42);
    CampoTexto campoClave("Contraseña", loginX, 295, 300, 42, true);
    Boton botonEntrar("Entrar", loginX, 365, 300, 46);
    campoUsuario.setEnfocado(true); // el cursor empieza en "Usuario"
    string mensajeLogin;

    // ---------------- Componentes de la pantalla PANEL ----------------
    // barra lateral
    Boton botonPedidos("Pedidos", 20, 110, 180, 44);
    Boton botonInventario("Inventario", 20, 165, 180, 44);
    Boton botonCaja("Caja", 20, 220, 180, 44);
    Boton botonSalir("Salir", 20, ALTO - 65, 180, 44);

    // vista PEDIDOS
    Boton botonFiltro("Ver: Activos", TABLA_X + TABLA_ANCHO - 140, 22, 140, 32);
    Boton botonAvanzar("Avanzar", 260, 615, 250, 42);
    Boton botonCompletar("Llevar a Entregado", 525, 615, 250, 42);
    Boton botonCancelar("Cancelar pedido", 790, 615, 200, 42);

    // vista INVENTARIO
    CampoTexto campoBuscar("Buscar por ID o nombre", 810, 40, 270, 34);
    CampoTexto campoNombre("Nombre del producto", 260, 460, 340, 38);
    CampoTexto campoPrecio("Precio ($)", 620, 460, 130, 38);
    CampoTexto campoStock("Existencias", 770, 460, 130, 38);
    Boton botonAgregar("Agregar a la lista", 260, 512, 220, 40);
    Boton botonGuardar("Guardar cambios", 490, 512, 190, 40);
    Boton botonEliminar("Eliminar", 690, 512, 240, 40);
    Boton botonLimpiar("Limpiar", 940, 512, 130, 40);
    Boton botonSubir("Subir a la base de datos", 260, 562, 350, 42);
    CampoTexto campoRestock("Cantidad a agregar al stock (reabastecer)", 260, 632, 150, 38);
    Boton botonRestock("Reabastecer", 430, 632, 170, 38);

    campoNombre.setMaxLongitud(40);
    campoPrecio.setMaxLongitud(8);
    campoStock.setMaxLongitud(5);
    campoBuscar.setMaxLongitud(20);
    campoRestock.setMaxLongitud(4);
    botonEliminar.setColor(ROJO_ERROR);
    botonSubir.setColor(VERDE);
    botonCancelar.setColor(ROJO_ERROR);

    // vista CAJA
    CampoTexto campoCaja("Cantidad vendida", 260, 488, 150, 38);
    Boton botonCobrar("Cobrar venta", 430, 488, 170, 38);

    // ---------------- Estado del programa ----------------
    Pantalla pantalla = Pantalla::LOGIN;
    Vista vista = Vista::PEDIDOS;
    int scroll = 0;              // primera fila visible de la tabla
    int scrollMaximo = 0;        // hasta donde se puede bajar
    string mensajePanel;         // errores de red mostrados abajo (al cargar datos)
    string mensajeAccion;        // resultado de la ultima accion (ok o error)
    bool accionOk = true;
    sf::Clock relojRefresco;     // para actualizar solo cada X segundos
    sf::Clock relojMensaje;      // para borrar el mensaje de accion despues de un rato
    sf::Clock relojConfirmar;    // para cancelar el "¿Seguro?" de Eliminar si no se confirma

    string productoSel;          // idProducto seleccionado (inventario / caja)
    int pendienteSel = -1;       // posicion del producto NUEVO seleccionado (inventario)
    string pedidoSel;            // folio seleccionado (pedidos)
    bool soloActivos = true;     // filtro de la vista PEDIDOS
    bool confirmandoEliminar = false;

    auto avisar = [&](const string& texto, bool ok) {
        mensajeAccion = texto;
        accionOk = ok;
        relojMensaje.restart();
    };

    // Busca el nombre de un producto en el inventario ya cargado.
    auto nombreProducto = [&](const string& id) -> string {
        for (const Producto& p : cafeteria.getInventario()) {
            if (p.getIdProducto() == id) return p.getNombreProducto();
        }
        return id;
    };

    // Estado actual de un pedido (segun la lista ya cargada); "" si no existe.
    auto estadoDePedido = [&](const string& folio) -> string {
        for (const Pedido& p : cafeteria.getListaPedidos()) {
            if (p.getFolio() == folio) return p.getEstado();
        }
        return "";
    };

    // Indices de los pedidos que se muestran (segun el filtro).
    auto filtrarPedidos = [&](const vector<Pedido>& todos) {
        vector<int> visibles;
        for (size_t i = 0; i < todos.size(); i++) {
            if (!soloActivos || estaActivo(todos[i].getEstado())) {
                visibles.push_back(static_cast<int>(i));
            }
        }
        return visibles;
    };

    // ---- Inventario: filas de la tabla = productos de la BD + productos NUEVOS sin subir ----
    struct FilaInv {
        bool pendiente;
        int indice; // posicion en el inventario (o en la lista de nuevos si pendiente)
    };

    auto filasInventario = [&]() {
        vector<FilaInv> filas;
        const vector<Producto> inv = cafeteria.getInventario();
        const vector<ProductoNuevo>& nuevos = cafeteria.getProductosPendientes();
        string busqueda = minusculas(campoBuscar.getContenido());

        for (size_t i = 0; i < inv.size(); i++) {
            if (busqueda.empty() ||
                minusculas(inv[i].getIdProducto()).find(busqueda) != string::npos ||
                minusculas(inv[i].getNombreProducto()).find(busqueda) != string::npos) {
                filas.push_back({false, static_cast<int>(i)});
            }
        }

        for (size_t i = 0; i < nuevos.size(); i++) {
            if (busqueda.empty() || minusculas(nuevos[i].nombre).find(busqueda) != string::npos) {
                filas.push_back({true, static_cast<int>(i)});
            }
        }

        return filas;
    };

    auto limpiarFormulario = [&]() {
        productoSel.clear();
        pendienteSel = -1;
        confirmandoEliminar = false;
        campoNombre.limpiar();
        campoPrecio.limpiar();
        campoStock.limpiar();
    };

    // Selecciona una fila de la tabla y pasa sus datos al formulario para editarlos.
    auto seleccionarFila = [&](const FilaInv& fila) {
        confirmandoEliminar = false;

        if (fila.pendiente) {
            const ProductoNuevo& n = cafeteria.getProductosPendientes()[fila.indice];
            productoSel.clear();
            pendienteSel = fila.indice;
            campoNombre.setContenido(n.nombre);
            campoPrecio.setContenido(precioTexto(n.precio));
            campoStock.setContenido(to_string(n.stock));
        } else {
            const vector<Producto> inv = cafeteria.getInventario();
            const Producto& p = inv[fila.indice];
            productoSel = p.getIdProducto();
            pendienteSel = -1;
            campoNombre.setContenido(p.getNombreProducto());
            campoPrecio.setContenido(precioTexto(p.getPrecio()));
            campoStock.setContenido(to_string(p.getStock()));
        }
    };

    // Pide al servidor los datos de la vista actual.
    //
    // Los pedidos SIEMPRE se refrescan, sin importar la pestana activa: asi
    // se detectan a tiempo (para la tabla de Pedidos, el punto de stock
    // agotado y la ganancia del turno) los pedidos que se completan desde
    // la web o desde el simulador de clientes mientras el encargado esta
    // viendo Inventario o Caja.
    auto cargarVista = [&]() {
        bool ok = cafeteria.cargarListaPedidos();

        if (vista == Vista::PEDIDOS) {
            if (ok && !pedidoSel.empty()) {
                if (estadoDePedido(pedidoSel).empty()) {
                    pedidoSel.clear();                       // ya no existe
                } else {
                    cafeteria.cargarDetallePedido(pedidoSel); // refresca sus productos
                }
            }
        } else {
            ok = cafeteria.cargarInventario() && ok;
        }

        mensajePanel = ok ? "" : ("Sin datos: " + cafeteria.getUltimoError());
        relojRefresco.restart();
    };

    auto cambiarVista = [&](Vista nueva) {
        vista = nueva;
        scroll = 0;
        mensajeAccion.clear();
        confirmandoEliminar = false;
        campoRestock.setEnfocado(false);
        campoCaja.setEnfocado(false);
        campoBuscar.setEnfocado(false);
        campoNombre.setEnfocado(false);
        campoPrecio.setEnfocado(false);
        campoStock.setEnfocado(false);
        cargarVista();
    };

    auto intentarLogin = [&]() {
        if (cafeteria.iniciarSesionCafeteria(campoUsuario.getContenido(), campoClave.getContenido())) {
            pantalla = Pantalla::PANEL;
            mensajeLogin.clear();
            cambiarVista(Vista::PEDIDOS);
        } else {
            mensajeLogin = cafeteria.getUltimoError();
            campoClave.limpiar();
        }
    };

    // ---- Acciones del panel ----
    auto hacerRestock = [&]() {
        if (productoSel.empty()) {
            avisar("Primero haz clic en un producto de la tabla.", false);
            return;
        }

        int cantidad = leerCantidad(campoRestock.getContenido());
        if (cantidad < 0) {
            avisar("Escribe una cantidad valida (1 a 9999).", false);
            return;
        }

        if (cafeteria.restockProducto(productoSel, cantidad)) {
            avisar("Se agregaron " + to_string(cantidad) + " a " +
                   recortar(nombreProducto(productoSel), 24) + ".", true);
            campoRestock.limpiar();

            // Si el producto esta en el formulario, refresca su stock ahi tambien.
            for (const Producto& p : cafeteria.getInventario()) {
                if (p.getIdProducto() == productoSel) {
                    campoStock.setContenido(to_string(p.getStock()));
                    break;
                }
            }
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto hacerCobro = [&]() {
        if (productoSel.empty()) {
            avisar("Primero haz clic en un producto de la tabla.", false);
            return;
        }

        int cantidad = leerCantidad(campoCaja.getContenido());
        if (cantidad < 0) {
            avisar("Escribe una cantidad valida (1 a 9999).", false);
            return;
        }

        if (cafeteria.venderEnCaja(productoSel, cantidad)) {
            const VentaCaja& v = cafeteria.getHistorialCaja().back();
            avisar("Venta: " + to_string(v.cantidad) + " x " + recortar(v.nombreProducto, 22) +
                   " = " + dinero(v.subtotal), true);
            campoCaja.limpiar();
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    // ---- Acciones del editor de inventario ----
    // Lee el formulario; regresa false (y avisa) si algo no sirve.
    auto leerFormulario = [&](string& nombre, float& precio, int& stock) -> bool {
        nombre = campoNombre.getContenido();
        precio = leerPrecio(campoPrecio.getContenido());
        stock = leerExistencias(campoStock.getContenido());

        if (nombre.empty()) { avisar("Escribe el nombre del producto.", false); return false; }
        if (precio < 0)     { avisar("Escribe un precio valido (mayor a 0, ej. 25.50).", false); return false; }
        if (stock < 0)      { avisar("Escribe las existencias (un numero entero).", false); return false; }

        // Un producto NUEVO (aun sin subir) no puede empezar con 0 existencias.
        if (stock < 1 && productoSel.empty()) {
            avisar("Un producto nuevo debe tener al menos 1 existencia inicial (no puede ser 0).", false);
            return false;
        }
        return true;
    };

    auto agregarALista = [&]() {
        string nombre; float precio; int stock;
        if (!leerFormulario(nombre, precio, stock)) return;

        if (cafeteria.agregarProductoPendiente(nombre, precio, stock)) {
            limpiarFormulario();
            campoBuscar.limpiar();
            avisar("Agregado a la lista. Pulsa \"Subir a la base de datos\" para guardarlo.", true);
            scroll = 1000000; // baja hasta el final para ver el producto nuevo (se limita solo)
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto guardarCambios = [&]() {
        if (productoSel.empty() && pendienteSel < 0) {
            avisar("Primero haz clic en el producto de la tabla que quieres modificar.", false);
            return;
        }

        string nombre; float precio; int stock;
        if (!leerFormulario(nombre, precio, stock)) return;

        if (pendienteSel >= 0) {
            if (cafeteria.modificarProductoPendiente(static_cast<size_t>(pendienteSel), nombre, precio, stock)) {
                avisar("Cambios guardados en la lista de nuevos (aun falta subirlos).", true);
            } else {
                avisar(cafeteria.getUltimoError(), false);
            }
            return;
        }

        string id = productoSel;
        if (cafeteria.modificarProducto(id, nombre, precio, stock)) {
            avisar("Producto " + id + " modificado en la base de datos.", true);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto eliminarSeleccion = [&]() {
        if (productoSel.empty() && pendienteSel < 0) {
            avisar("Primero haz clic en el producto de la tabla que quieres eliminar.", false);
            return;
        }

        // Un producto NUEVO todavia no esta en la base de datos: se quita sin preguntar.
        if (pendienteSel >= 0) {
            cafeteria.quitarProductoPendiente(static_cast<size_t>(pendienteSel));
            limpiarFormulario();
            avisar("Producto nuevo quitado de la lista.", true);
            return;
        }

        // Un producto de la base de datos: pide confirmacion (segundo clic).
        if (!confirmandoEliminar) {
            confirmandoEliminar = true;
            relojConfirmar.restart();
            avisar("Vuelve a pulsar Eliminar para confirmar (se borra de la base de datos).", false);
            return;
        }

        confirmandoEliminar = false;
        string id = productoSel;
        string nombre = nombreProducto(id);

        if (cafeteria.eliminarProducto(id)) {
            limpiarFormulario();
            avisar("Producto " + id + " (" + recortar(nombre, 20) + ") eliminado.", true);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto subirNuevos = [&]() {
        int total = static_cast<int>(cafeteria.getProductosPendientes().size());

        if (total == 0) {
            avisar("No hay productos nuevos por subir.", false);
            return;
        }

        int subidos = cafeteria.subirProductosPendientes();
        pendienteSel = -1;
        confirmandoEliminar = false;

        if (subidos == total) {
            limpiarFormulario();
            avisar("Se subieron " + to_string(subidos) + " producto(s) a la base de datos.", true);
        } else if (subidos > 0) {
            avisar("Se subieron " + to_string(subidos) + " de " + to_string(total) +
                   ". " + cafeteria.getUltimoError(), false);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    // Enter en la barra de busqueda: si solo queda UN producto, lo selecciona.
    auto seleccionarBusqueda = [&]() {
        vector<FilaInv> filas = filasInventario();

        if (filas.size() == 1) {
            seleccionarFila(filas[0]);
            avisar("Producto seleccionado.", true);
        } else if (filas.empty()) {
            avisar("No se encontro ningun producto con ese ID o nombre.", false);
        } else {
            avisar("Hay " + to_string(filas.size()) + " coincidencias: haz clic en la que quieras.", true);
        }
    };

    auto avanzarPedido = [&]() {
        string texto, nuevo;
        if (pedidoSel.empty() || !siguienteEstado(estadoDePedido(pedidoSel), texto, nuevo)) {
            return;
        }

        if (cafeteria.cambiarEstadoPedido(pedidoSel, nuevo)) {
            avisar("Pedido " + recortar(pedidoSel, 16) + ": " + nuevo + ".", true);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto completarPedidoSel = [&]() {
        if (pedidoSel.empty() || !estaActivo(estadoDePedido(pedidoSel))) {
            return;
        }

        if (cafeteria.completarPedido(pedidoSel)) {
            avisar("Pedido " + recortar(pedidoSel, 16) + ": Entregado.", true);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    auto cancelarPedidoSel = [&]() {
        if (pedidoSel.empty() || !sePuedeCancelar(estadoDePedido(pedidoSel))) {
            return;
        }

        if (cafeteria.cancelarPedido(pedidoSel)) {
            avisar("Pedido " + recortar(pedidoSel, 16) + " cancelado.", true);
        } else {
            avisar(cafeteria.getUltimoError(), false);
        }
    };

    // Devuelve el indice absoluto de la fila donde se hizo clic (o -1).
    auto filaClicada = [&](float mx, float my, int filasVisibles) -> int {
        if (mx < TABLA_X || mx > TABLA_X + TABLA_ANCHO) return -1;

        float relativo = my - (TABLA_Y_FILAS - 3);
        if (relativo < 0) return -1;

        int i = static_cast<int>(relativo / ALTO_FILA);
        if (i >= filasVisibles) return -1;

        return scroll + i;
    };

    // Enfoca solo UN campo del formulario / busqueda / restock (los demas pierden el foco).
    auto enfocarCampoInventario = [&](float mx, float my) {
        campoBuscar.setEnfocado(campoBuscar.contiene(mx, my));
        campoNombre.setEnfocado(campoNombre.contiene(mx, my));
        campoPrecio.setEnfocado(campoPrecio.contiene(mx, my));
        campoStock.setEnfocado(campoStock.contiene(mx, my));
        campoRestock.setEnfocado(campoRestock.contiene(mx, my));
    };

    // =================================================================
    //                        CICLO PRINCIPAL
    // =================================================================
    while (ventana.estaAbierta()) {

        // ---------- 1) EVENTOS ----------
        sf::Event e;
        while (win.pollEvent(e)) {

            if (e.type == sf::Event::Closed) {
                ventana.cerrar();
                break;
            }

            if (pantalla == Pantalla::LOGIN) {

                if (e.type == sf::Event::MouseMoved) {
                    botonEntrar.actualizarHover(static_cast<float>(e.mouseMove.x),
                                                static_cast<float>(e.mouseMove.y));
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    // Clic en un campo = darle el foco (el otro lo pierde)
                    campoUsuario.setEnfocado(campoUsuario.contiene(mx, my));
                    campoClave.setEnfocado(campoClave.contiene(mx, my));

                    if (botonEntrar.contiene(mx, my)) {
                        intentarLogin();
                    }
                }
                else if (e.type == sf::Event::TextEntered) {
                    // Solo el campo que tiene el foco hace caso
                    campoUsuario.recibirCaracter(e.text.unicode);
                    campoClave.recibirCaracter(e.text.unicode);
                }
                else if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Tab) {
                        bool enUsuario = campoUsuario.estaEnfocado();
                        campoUsuario.setEnfocado(!enUsuario);
                        campoClave.setEnfocado(enUsuario);
                    }
                    else if (e.key.code == sf::Keyboard::Return) {
                        intentarLogin();
                    }
                }
            }
            else { // Pantalla::PANEL

                if (e.type == sf::Event::MouseMoved) {
                    float mx = static_cast<float>(e.mouseMove.x);
                    float my = static_cast<float>(e.mouseMove.y);

                    botonPedidos.actualizarHover(mx, my);
                    botonInventario.actualizarHover(mx, my);
                    botonCaja.actualizarHover(mx, my);
                    botonSalir.actualizarHover(mx, my);

                    botonFiltro.actualizarHover(mx, my);
                    botonAvanzar.actualizarHover(mx, my);
                    botonCompletar.actualizarHover(mx, my);
                    botonCancelar.actualizarHover(mx, my);

                    botonAgregar.actualizarHover(mx, my);
                    botonGuardar.actualizarHover(mx, my);
                    botonEliminar.actualizarHover(mx, my);
                    botonLimpiar.actualizarHover(mx, my);
                    botonSubir.actualizarHover(mx, my);
                    botonRestock.actualizarHover(mx, my);

                    botonCobrar.actualizarHover(mx, my);
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    // ---- barra lateral (igual en todas las vistas) ----
                    if (botonPedidos.contiene(mx, my))         cambiarVista(Vista::PEDIDOS);
                    else if (botonInventario.contiene(mx, my)) cambiarVista(Vista::INVENTARIO);
                    else if (botonCaja.contiene(mx, my))       cambiarVista(Vista::CAJA);
                    else if (botonSalir.contiene(mx, my))      ventana.cerrar();

                    // ---- zona de contenido (depende de la vista) ----
                    else if (vista == Vista::INVENTARIO) {
                        enfocarCampoInventario(mx, my);

                        bool hayNuevos = !cafeteria.getProductosPendientes().empty();

                        if (botonAgregar.contiene(mx, my))                    agregarALista();
                        else if (botonGuardar.contiene(mx, my))               guardarCambios();
                        else if (botonEliminar.contiene(mx, my))              eliminarSeleccion();
                        else if (botonLimpiar.contiene(mx, my))               limpiarFormulario();
                        else if (hayNuevos && botonSubir.contiene(mx, my))    subirNuevos();
                        else if (botonRestock.contiene(mx, my))               hacerRestock();
                        else {
                            int fila = filaClicada(mx, my, FILAS_INVENTARIO);
                            vector<FilaInv> filas = filasInventario();
                            if (fila >= 0 && fila < static_cast<int>(filas.size())) {
                                seleccionarFila(filas[fila]);
                            }
                        }
                    }
                    else if (vista == Vista::CAJA) {
                        campoCaja.setEnfocado(campoCaja.contiene(mx, my));

                        if (botonCobrar.contiene(mx, my)) {
                            hacerCobro();
                        } else {
                            int fila = filaClicada(mx, my, FILAS_CAJA);
                            vector<Producto> inv = cafeteria.getInventario();
                            if (fila >= 0 && fila < static_cast<int>(inv.size())) {
                                productoSel = inv[fila].getIdProducto();
                            }
                        }
                    }
                    else { // Vista::PEDIDOS
                        string estadoSel = pedidoSel.empty() ? "" : estadoDePedido(pedidoSel);
                        string textoAvanzar, estadoSiguiente;
                        bool hayAvance = siguienteEstado(estadoSel, textoAvanzar, estadoSiguiente);
                        bool hayCompletar = (estadoSel == "Pendiente" || estadoSel == "Preparando");

                        if (botonFiltro.contiene(mx, my)) {
                            soloActivos = !soloActivos;
                            botonFiltro.setTexto(soloActivos ? "Ver: Activos" : "Ver: Todos");
                            scroll = 0;
                        }
                        else if (hayAvance && botonAvanzar.contiene(mx, my)) {
                            avanzarPedido();
                        }
                        else if (hayCompletar && botonCompletar.contiene(mx, my)) {
                            completarPedidoSel();
                        }
                        else if (sePuedeCancelar(estadoSel) && botonCancelar.contiene(mx, my)) {
                            cancelarPedidoSel();
                        }
                        else {
                            vector<Pedido> todos = cafeteria.getListaPedidos();
                            vector<int> visibles = filtrarPedidos(todos);
                            int fila = filaClicada(mx, my, FILAS_PEDIDOS);

                            if (fila >= 0 && fila < static_cast<int>(visibles.size())) {
                                pedidoSel = todos[visibles[fila]].getFolio();
                                cafeteria.cargarDetallePedido(pedidoSel);
                            }
                        }
                    }
                }
                else if (e.type == sf::Event::TextEntered) {
                    unsigned int u = e.text.unicode;
                    bool esDigito = (u >= '0' && u <= '9');

                    if (vista == Vista::INVENTARIO) {
                        if (campoNombre.estaEnfocado()) {
                            if (u != '|') campoNombre.recibirCaracter(u);  // '|' separa los campos del protocolo
                        }
                        else if (campoPrecio.estaEnfocado()) {
                            if (u == 8 || esDigito) {
                                campoPrecio.recibirCaracter(u);
                            } else if ((u == '.' || u == ',') &&
                                       campoPrecio.getContenido().find('.') == string::npos) {
                                campoPrecio.recibirCaracter('.');
                            }
                        }
                        else if (campoStock.estaEnfocado()) {
                            if (u == 8 || esDigito) campoStock.recibirCaracter(u);
                        }
                        else if (campoRestock.estaEnfocado()) {
                            if (u == 8 || esDigito) campoRestock.recibirCaracter(u);
                        }
                        else if (campoBuscar.estaEnfocado()) {
                            if (u != '|') {
                                campoBuscar.recibirCaracter(u);
                                scroll = 0;
                            }
                        }
                    }
                    else if (vista == Vista::CAJA) {
                        // Solo numeros (y borrar) en el campo de cantidad
                        if (u == 8 || esDigito) campoCaja.recibirCaracter(u);
                    }
                }
                else if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Tab) {
                    // Tab: pasa al siguiente campo del formulario (Nombre -> Precio -> Existencias)
                    if (vista == Vista::INVENTARIO) {
                        if (campoNombre.estaEnfocado()) {
                            campoNombre.setEnfocado(false);
                            campoPrecio.setEnfocado(true);
                        } else if (campoPrecio.estaEnfocado()) {
                            campoPrecio.setEnfocado(false);
                            campoStock.setEnfocado(true);
                        } else if (campoStock.estaEnfocado()) {
                            campoStock.setEnfocado(false);
                            campoNombre.setEnfocado(true);
                        }
                    }
                }
                else if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Return) {
                    if (vista == Vista::INVENTARIO) {
                        if (campoRestock.estaEnfocado()) {
                            hacerRestock();
                        } else if (campoBuscar.estaEnfocado()) {
                            seleccionarBusqueda();
                        } else if (campoNombre.estaEnfocado() || campoPrecio.estaEnfocado() ||
                                   campoStock.estaEnfocado()) {
                            if (!productoSel.empty() || pendienteSel >= 0) guardarCambios();
                            else agregarALista();
                        }
                    }
                    else if (vista == Vista::CAJA && campoCaja.estaEnfocado()) {
                        hacerCobro();
                    }
                }
                else if (e.type == sf::Event::MouseWheelScrolled) {
                    // rueda hacia arriba (delta > 0) = subir en la tabla
                    scroll -= static_cast<int>(e.mouseWheelScroll.delta);
                    scroll = max(0, min(scroll, scrollMaximo));
                }
            }
        }

        if (!ventana.estaAbierta()) {
            break;
        }

        // ---------- 2) ACTUALIZAR DATOS (solo cada 3 segundos) ----------
        // Asi los pedidos nuevos aparecen "casi en tiempo real", sin hilos
        // ni nada complicado: es el mismo ciclo, con un reloj.
        if (pantalla == Pantalla::PANEL &&
            relojRefresco.getElapsedTime().asSeconds() >= SEGUNDOS_REFRESCO) {
            cargarVista();
        }

        // El mensaje de la ultima accion desaparece solo despues de unos segundos.
        if (!mensajeAccion.empty() && relojMensaje.getElapsedTime().asSeconds() >= SEGUNDOS_MENSAJE) {
            mensajeAccion.clear();
        }

        // Si no se confirma "Eliminar" a tiempo, se cancela la confirmacion.
        if (confirmandoEliminar && relojConfirmar.getElapsedTime().asSeconds() >= SEGUNDOS_CONFIRMAR) {
            confirmandoEliminar = false;
        }

        // Si el producto seleccionado ya no existe (lo borraron desde otro lado), se deselecciona.
        if (pantalla == Pantalla::PANEL && !productoSel.empty() && vista != Vista::PEDIDOS) {
            bool existe = false;
            for (const Producto& p : cafeteria.getInventario()) {
                if (p.getIdProducto() == productoSel) { existe = true; break; }
            }
            if (!existe && !cafeteria.getInventario().empty()) {
                productoSel.clear();
                confirmandoEliminar = false;
            }
        }

        // ---------- 3) DIBUJAR ----------
        win.clear(FONDO);

        if (pantalla == Pantalla::LOGIN) {

            ventana.dibujarTextoCentrado("UPIIFOOD", sf::FloatRect(0, 70, ANCHO, 60), 52, BARRA_LATERAL);
            ventana.dibujarTextoCentrado("Acceso de cafetería", sf::FloatRect(0, 135, ANCHO, 30), 22, TEXTO_SUAVE);

            campoUsuario.dibujar(ventana);
            campoClave.dibujar(ventana);
            botonEntrar.dibujar(ventana);

            if (!mensajeLogin.empty()) {
                ventana.dibujarTextoCentrado(mensajeLogin, sf::FloatRect(0, 435, ANCHO, 30), 18, ROJO_ERROR);
            }

            ventana.dibujarTextoCentrado("Servidor: " + ip, sf::FloatRect(0, ALTO - 40, ANCHO, 24), 14, TEXTO_SUAVE);
        }
        else {
            // --- barra lateral ---
            rectangulo(ventana, 0, 0, 220, ALTO, BARRA_LATERAL);
            ventana.dibujarTexto("UPIIFOOD", 20, 25, 26, CREMA);
            ventana.dibujarTexto("Cafetería", 20, 62, 16, sf::Color(200, 180, 160));

            botonPedidos.setActivo(vista == Vista::PEDIDOS);
            botonInventario.setActivo(vista == Vista::INVENTARIO);
            botonCaja.setActivo(vista == Vista::CAJA);

            botonPedidos.dibujar(ventana);
            botonInventario.dibujar(ventana);
            botonCaja.dibujar(ventana);
            botonSalir.dibujar(ventana);

            // puntito rojo sobre "Inventario": alerta de producto agotado (stock 0)
            if (cafeteria.hayProductoAgotado()) {
                sf::CircleShape avisoAgotado(7);
                avisoAgotado.setFillColor(ROJO_ERROR);
                avisoAgotado.setPosition(20 + 180 - 14, 165 - 6);
                win.draw(avisoAgotado);
            }

            ventana.dibujarTexto("Sesión de:", 20, ALTO - 150, 13, sf::Color(200, 180, 160));
            ventana.dibujarTexto(recortar(cafeteria.getNombreCafeteria(), 18), 20, ALTO - 130, 16, CREMA);
            ventana.dibujarTexto("Cafetería " + cafeteria.getIdCafeteria(), 20, ALTO - 108, 13, sf::Color(200, 180, 160));

            // --- zona de contenido ---
            rectangulo(ventana, 240, 15, 850, ALTO - 30, sf::Color(255, 255, 255));

            // ===================== VISTA PEDIDOS =====================
            if (vista == Vista::PEDIDOS) {
                const vector<Pedido> todos = cafeteria.getListaPedidos();
                const vector<int> visibles = filtrarPedidos(todos);
                int total = static_cast<int>(visibles.size());
                scrollMaximo = max(0, total - FILAS_PEDIDOS);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Pedidos (" + to_string(total) + ")",
                                "Haz clic en un pedido para ver su detalle.",
                                {{"Folio", 260}, {"Cliente", 450}, {"Estado", 650}, {"Total", 900}});

                botonFiltro.dibujar(ventana);

                for (int i = 0; i < FILAS_PEDIDOS && scroll + i < total; i++) {
                    const Pedido& p = todos[visibles[scroll + i]];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i, p.getFolio() == pedidoSel);
                    ventana.dibujarTexto(recortar(p.getFolio(), 18), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(p.getUsernameCliente(), 18), 450, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.getEstado(), 650, y, 18, colorEstado(p.getEstado()));
                    ventana.dibujarTexto(dinero(p.getTotal()), 900, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto(soloActivos ? "No hay pedidos activos por ahora."
                                                     : "Todavía no hay pedidos.",
                                         260, 150, 18, TEXTO_SUAVE);
                }

                // --- detalle del pedido seleccionado ---
                rectangulo(ventana, TABLA_X, 392, TABLA_ANCHO, 2, sf::Color(200, 190, 178));

                const Pedido* seleccionado = nullptr;
                for (const Pedido& p : todos) {
                    if (p.getFolio() == pedidoSel) { seleccionado = &p; break; }
                }

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Detalle del pedido", 260, 405, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto("Selecciona un pedido de la tabla para ver sus productos y cambiar su estado.",
                                         260, 435, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Pedido " + recortar(seleccionado->getFolio(), 30), 260, 403, 20, TEXTO_OSCURO);
                    ventana.dibujarTexto("Cliente: " + recortar(seleccionado->getUsernameCliente(), 24) +
                                         "   ·   Fecha: " + seleccionado->getFecha(),
                                         260, 433, 16, TEXTO_SUAVE);

                    ventana.dibujarTexto("Estado:", 260, 458, 17, TEXTO_SUAVE);
                    ventana.dibujarTexto(seleccionado->getEstado(), 335, 458, 17, colorEstado(seleccionado->getEstado()));
                    ventana.dibujarTexto("Total: " + dinero(seleccionado->getTotal()), 620, 458, 17, TEXTO_OSCURO);

                    const vector<pair<Producto, int>>& detalle = cafeteria.getDetallePedido();
                    bool detalleVigente = (cafeteria.getFolioDetalle() == pedidoSel);
                    int maxLineas = 6;

                    if (!detalleVigente) {
                        ventana.dibujarTexto("Cargando productos...", 260, 490, 16, TEXTO_SUAVE);
                    }

                    for (int i = 0; detalleVigente && i < static_cast<int>(detalle.size()); i++) {
                        float y = 490.0f + i * 20.0f;

                        if (i == maxLineas - 1 && static_cast<int>(detalle.size()) > maxLineas) {
                            ventana.dibujarTexto("... y " + to_string(detalle.size() - i) + " productos más",
                                                 260, y, 16, TEXTO_SUAVE);
                            break;
                        }

                        const Producto& prod = detalle[i].first;
                        int cantidad = detalle[i].second;

                        ventana.dibujarTexto(to_string(cantidad) + " x " + recortar(prod.getNombreProducto(), 40),
                                             260, y, 16, TEXTO_OSCURO);
                        ventana.dibujarTexto(dinero(prod.getPrecio() * cantidad), 900, y, 16, TEXTO_SUAVE);
                    }

                    // --- botones de accion (solo los que tienen sentido para este estado) ---
                    string textoAvanzar, estadoSiguiente;
                    if (siguienteEstado(seleccionado->getEstado(), textoAvanzar, estadoSiguiente)) {
                        botonAvanzar.setTexto(textoAvanzar);
                        botonAvanzar.dibujar(ventana);
                    }

                    if (seleccionado->getEstado() == "Pendiente" || seleccionado->getEstado() == "Preparando") {
                        botonCompletar.dibujar(ventana);
                    }

                    if (sePuedeCancelar(seleccionado->getEstado())) {
                        botonCancelar.dibujar(ventana);
                    }
                }
            }
            // ==================== VISTA INVENTARIO ====================
            else if (vista == Vista::INVENTARIO) {
                const vector<Producto> inv = cafeteria.getInventario();
                const vector<ProductoNuevo>& nuevos = cafeteria.getProductosPendientes();
                const vector<FilaInv> filas = filasInventario();
                int total = static_cast<int>(filas.size());
                scrollMaximo = max(0, total - FILAS_INVENTARIO);
                scroll = max(0, min(scroll, scrollMaximo));

                string titulo = "Inventario (" + to_string(inv.size()) + ")";
                if (!nuevos.empty()) titulo += "  +" + to_string(nuevos.size()) + " nuevo(s)";

                encabezadoTabla(ventana, titulo,
                                "Clic en un producto para editarlo. Rueda del mouse para bajar.",
                                {{"ID", 260}, {"Producto", 350}, {"Precio", 700}, {"Stock", 860}});

                campoBuscar.dibujar(ventana);

                for (int i = 0; i < FILAS_INVENTARIO && scroll + i < total; i++) {
                    const FilaInv& fila = filas[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    if (fila.pendiente) {
                        const ProductoNuevo& n = nuevos[fila.indice];
                        bool esSel = (fila.indice == pendienteSel);

                        franja(ventana, i, esSel, true);
                        ventana.dibujarTexto("NUEVO", 260, y, 18, AMBAR);
                        ventana.dibujarTexto(recortar(n.nombre, 30), 350, y, 18, TEXTO_OSCURO);
                        ventana.dibujarTexto(dinero(n.precio), 700, y, 18, TEXTO_OSCURO);
                        ventana.dibujarTexto(to_string(n.stock) + " (sin subir)", 860, y, 18, AMBAR);
                    } else {
                        const Producto& p = inv[fila.indice];
                        bool esSel = (p.getIdProducto() == productoSel);

                        franja(ventana, i, esSel);
                        ventana.dibujarTexto(p.getIdProducto(), 260, y, 18, TEXTO_SUAVE);
                        ventana.dibujarTexto(recortar(p.getNombreProducto(), 30), 350, y, 18, TEXTO_OSCURO);
                        ventana.dibujarTexto(dinero(p.getPrecio()), 700, y, 18, TEXTO_OSCURO);

                        if (p.getStock() == 0) {
                            ventana.dibujarTexto("Agotado", 860, y, 18, ROJO_ERROR);
                        } else if (p.getStock() <= 5) {
                            ventana.dibujarTexto(to_string(p.getStock()) + " (bajo)", 860, y, 18, ROJO_ERROR);
                        } else {
                            ventana.dibujarTexto(to_string(p.getStock()), 860, y, 18, TEXTO_OSCURO);
                        }
                    }
                }

                if (total == 0 && mensajePanel.empty()) {
                    bool buscando = !campoBuscar.getContenido().empty();
                    ventana.dibujarTexto(buscando ? "No hay productos con ese ID o nombre."
                                                  : "Esta cafetería todavía no tiene productos. ¡Agrega el primero abajo!",
                                         260, 150, 18, TEXTO_SUAVE);
                }

                // --- editor ---
                rectangulo(ventana, TABLA_X, 392, TABLA_ANCHO, 2, sf::Color(200, 190, 178));

                if (!productoSel.empty()) {
                    ventana.dibujarTexto("Editando: " + productoSel + "  (" + recortar(nombreProducto(productoSel), 30) +
                                         ")   ·   el ID lo genera el sistema y no se puede cambiar",
                                         260, 402, 16, TEXTO_OSCURO);
                } else if (pendienteSel >= 0) {
                    ventana.dibujarTexto("Editando un producto NUEVO (todavía no está en la base de datos).",
                                         260, 402, 16, AMBAR);
                } else {
                    ventana.dibujarTexto("Producto nuevo: llena los campos (existencias: mínimo 1) y pulsa Agregar. "
                                         "Para editar uno, clic en la tabla.",
                                         260, 402, 16, TEXTO_SUAVE);
                }

                campoNombre.dibujar(ventana);
                campoPrecio.dibujar(ventana);
                campoStock.dibujar(ventana);

                botonAgregar.dibujar(ventana);
                botonGuardar.dibujar(ventana);
                botonEliminar.setTexto(confirmandoEliminar ? "¿Seguro? Clic de nuevo" : "Eliminar");
                botonEliminar.dibujar(ventana);
                botonLimpiar.dibujar(ventana);

                if (!nuevos.empty()) {
                    botonSubir.setTexto("Subir a la base de datos (" + to_string(nuevos.size()) + ")");
                    botonSubir.dibujar(ventana);
                    ventana.dibujarTexto("Los productos marcados NUEVO aún no están en la base de datos.",
                                         630, 574, 15, AMBAR);
                }

                campoRestock.dibujar(ventana);
                botonRestock.dibujar(ventana);
            }
            // ======================= VISTA CAJA =======================
            else {
                const vector<Producto> inv = cafeteria.getInventario();
                int total = static_cast<int>(inv.size());
                scrollMaximo = max(0, total - FILAS_CAJA);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Caja",
                                "Venta directa en mostrador: elige el producto y la cantidad.",
                                {{"ID", 260}, {"Producto", 350}, {"Precio", 700}, {"Stock", 860}});

                const Producto* seleccionado = nullptr;

                for (int i = 0; i < FILAS_CAJA && scroll + i < total; i++) {
                    const Producto& p = inv[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i, p.getIdProducto() == productoSel);
                    ventana.dibujarTexto(p.getIdProducto(), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(p.getNombreProducto(), 30), 350, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(dinero(p.getPrecio()), 700, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.getStock() == 0 ? "Agotado" : to_string(p.getStock()),
                                         860, y, 18, p.getStock() == 0 ? ROJO_ERROR : TEXTO_OSCURO);
                }

                for (const Producto& p : inv) {
                    if (p.getIdProducto() == productoSel) { seleccionado = &p; break; }
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Esta cafetería todavía no tiene productos.", 260, 150, 18, TEXTO_SUAVE);
                }

                rectangulo(ventana, TABLA_X, 422, TABLA_ANCHO, 2, sf::Color(200, 190, 178));

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Selecciona un producto de la tabla.", 260, 434, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Seleccionado: " + recortar(seleccionado->getNombreProducto(), 30) +
                                         "   " + dinero(seleccionado->getPrecio()) + " c/u",
                                         260, 434, 16, TEXTO_OSCURO);
                }

                campoCaja.dibujar(ventana);
                botonCobrar.dibujar(ventana);

                // ganancia del turno (a la derecha)
                ventana.dibujarTexto("Ganancia del turno", 800, 462, 15, TEXTO_SUAVE);
                ventana.dibujarTexto(dinero(cafeteria.getGananciaCajaTurno()), 800, 484, 32, VERDE);

                // ultimas ventas
                const vector<VentaCaja>& ventas = cafeteria.getHistorialCaja();
                ventana.dibujarTexto("Últimas ventas", 260, 560, 15, TEXTO_SUAVE);

                if (ventas.empty()) {
                    ventana.dibujarTexto("Todavía no hay ventas en este turno.", 260, 584, 15, TEXTO_SUAVE);
                } else {
                    for (int k = 0; k < 4 && k < static_cast<int>(ventas.size()); k++) {
                        const VentaCaja& v = ventas[ventas.size() - 1 - k];
                        ventana.dibujarTexto(to_string(v.cantidad) + " x " + recortar(v.nombreProducto, 34) +
                                             "  -  " + dinero(v.subtotal),
                                             260, 584.0f + k * 20.0f, 15, TEXTO_OSCURO);
                    }
                }
            }

            // --- mensaje de la ultima accion, o error de red (si lo hay) ---
            float yMensaje = ALTO - 42.0f;

            if (!mensajeAccion.empty()) {
                ventana.dibujarTexto(mensajeAccion, 260, yMensaje, 16, accionOk ? VERDE : ROJO_ERROR);
            } else if (!mensajePanel.empty()) {
                ventana.dibujarTexto(mensajePanel, 260, yMensaje, 16, ROJO_ERROR);
            }
        }

        win.display();
    }

    // Al salir del ciclo, el destructor de "cafeteria" cierra el socket y el
    // servidor nos marca automaticamente como desconectados.
    return 0;
}
