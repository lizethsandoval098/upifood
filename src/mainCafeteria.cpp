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
//                  (Pendiente -> Preparando -> Listo -> Entregado) o se cancelan.
//     INVENTARIO - existencias; se puede reabastecer un producto.
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
static const int ANCHO = 900;
static const int ALTO = 600;
static const float TABLA_Y_FILAS = 140.0f;
static const float ALTO_FILA = 30.0f;
static const int FILAS_INVENTARIO = 10;
static const int FILAS_CAJA = 8;
static const int FILAS_PEDIDOS = 6;
static const float SEGUNDOS_REFRESCO = 3.0f;
static const float SEGUNDOS_MENSAJE = 4.0f;

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

    rectangulo(v, 250, 130, 620, 2, sf::Color(200, 190, 178));
}

// Franja de fondo de la fila i (alternando color para que se lea mejor;
// la fila seleccionada se pinta de otro color).
static void franja(Ventana& v, int i, bool seleccionada = false) {
    float y = TABLA_Y_FILAS + i * ALTO_FILA;
    sf::Color color = seleccionada ? FILA_SELECCION
                                   : ((i % 2 == 0) ? FILA_CLARA : FILA_OSCURA);
    rectangulo(v, 250, y - 3, 620, ALTO_FILA - 2, color);
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
    CampoTexto campoUsuario("Usuario", 300, 215, 300, 42);
    CampoTexto campoClave("Contraseña", 300, 295, 300, 42, true);
    Boton botonEntrar("Entrar", 300, 365, 300, 46);
    campoUsuario.setEnfocado(true); // el cursor empieza en "Usuario"
    string mensajeLogin;

    // ---------------- Componentes de la pantalla PANEL ----------------
    // barra lateral
    Boton botonPedidos("Pedidos", 20, 110, 180, 44);
    Boton botonInventario("Inventario", 20, 165, 180, 44);
    Boton botonCaja("Caja", 20, 220, 180, 44);
    Boton botonActualizar("Actualizar ahora", 20, 300, 180, 44);
    Boton botonSalir("Salir", 20, 535, 180, 44);

    // vista PEDIDOS
    Boton botonFiltro("Ver: Activos", 730, 22, 140, 32);
    Boton botonAvanzar("Avanzar", 260, 522, 230, 40);
    Boton botonCancelar("Cancelar pedido", 510, 522, 200, 40);

    // vista INVENTARIO
    CampoTexto campoRestock("Cantidad a agregar", 260, 500, 150, 38);
    Boton botonRestock("Reabastecer", 430, 500, 170, 38);

    // vista CAJA
    CampoTexto campoCaja("Cantidad vendida", 260, 452, 150, 38);
    Boton botonCobrar("Cobrar venta", 430, 452, 170, 38);

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

    string productoSel;          // idProducto seleccionado (inventario / caja)
    string pedidoSel;            // folio seleccionado (pedidos)
    bool soloActivos = true;     // filtro de la vista PEDIDOS

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

    // Pide al servidor los datos de la vista actual.
    auto cargarVista = [&]() {
        bool ok = false;

        if (vista == Vista::PEDIDOS) {
            ok = cafeteria.cargarListaPedidos();

            if (ok && !pedidoSel.empty()) {
                if (estadoDePedido(pedidoSel).empty()) {
                    pedidoSel.clear();                       // ya no existe
                } else {
                    cafeteria.cargarDetallePedido(pedidoSel); // refresca sus productos
                }
            }
        } else {
            ok = cafeteria.cargarInventario();
        }

        mensajePanel = ok ? "" : ("Sin datos: " + cafeteria.getUltimoError());
        relojRefresco.restart();
    };

    auto cambiarVista = [&](Vista nueva) {
        vista = nueva;
        scroll = 0;
        mensajeAccion.clear();
        campoRestock.setEnfocado(false);
        campoCaja.setEnfocado(false);
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
        if (mx < 250 || mx > 870) return -1;

        float relativo = my - (TABLA_Y_FILAS - 3);
        if (relativo < 0) return -1;

        int i = static_cast<int>(relativo / ALTO_FILA);
        if (i >= filasVisibles) return -1;

        return scroll + i;
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
                    botonActualizar.actualizarHover(mx, my);
                    botonSalir.actualizarHover(mx, my);

                    botonFiltro.actualizarHover(mx, my);
                    botonAvanzar.actualizarHover(mx, my);
                    botonCancelar.actualizarHover(mx, my);
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
                    else if (botonActualizar.contiene(mx, my)) cargarVista();
                    else if (botonSalir.contiene(mx, my))      ventana.cerrar();

                    // ---- zona de contenido (depende de la vista) ----
                    else if (vista == Vista::INVENTARIO) {
                        campoRestock.setEnfocado(campoRestock.contiene(mx, my));

                        if (botonRestock.contiene(mx, my)) {
                            hacerRestock();
                        } else {
                            int fila = filaClicada(mx, my, FILAS_INVENTARIO);
                            vector<Producto> inv = cafeteria.getInventario();
                            if (fila >= 0 && fila < static_cast<int>(inv.size())) {
                                productoSel = inv[fila].getIdProducto();
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

                        if (botonFiltro.contiene(mx, my)) {
                            soloActivos = !soloActivos;
                            botonFiltro.setTexto(soloActivos ? "Ver: Activos" : "Ver: Todos");
                            scroll = 0;
                        }
                        else if (hayAvance && botonAvanzar.contiene(mx, my)) {
                            avanzarPedido();
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
                    // Solo numeros (y borrar) en los campos de cantidad
                    unsigned int u = e.text.unicode;

                    if (u == 8 || (u >= '0' && u <= '9')) {
                        if (vista == Vista::INVENTARIO)  campoRestock.recibirCaracter(u);
                        else if (vista == Vista::CAJA)   campoCaja.recibirCaracter(u);
                    }
                }
                else if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Return) {
                    if (vista == Vista::INVENTARIO && campoRestock.estaEnfocado()) hacerRestock();
                    else if (vista == Vista::CAJA && campoCaja.estaEnfocado())     hacerCobro();
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

            ventana.dibujarTextoCentrado("Servidor: " + ip, sf::FloatRect(0, 560, ANCHO, 24), 14, TEXTO_SUAVE);
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
            botonActualizar.dibujar(ventana);
            botonSalir.dibujar(ventana);

            ventana.dibujarTexto("Sesión de:", 20, 470, 13, sf::Color(200, 180, 160));
            ventana.dibujarTexto(recortar(cafeteria.getNombreCafeteria(), 18), 20, 490, 16, CREMA);
            ventana.dibujarTexto("Cafetería " + cafeteria.getIdCafeteria(), 20, 512, 13, sf::Color(200, 180, 160));

            // --- zona de contenido ---
            rectangulo(ventana, 240, 15, 650, 570, sf::Color(255, 255, 255));

            // ===================== VISTA PEDIDOS =====================
            if (vista == Vista::PEDIDOS) {
                const vector<Pedido> todos = cafeteria.getListaPedidos();
                const vector<int> visibles = filtrarPedidos(todos);
                int total = static_cast<int>(visibles.size());
                scrollMaximo = max(0, total - FILAS_PEDIDOS);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Pedidos (" + to_string(total) + ")",
                                "Haz clic en un pedido para ver su detalle.",
                                {{"Folio", 260}, {"Cliente", 410}, {"Estado", 550}, {"Total", 740}});

                botonFiltro.dibujar(ventana);

                for (int i = 0; i < FILAS_PEDIDOS && scroll + i < total; i++) {
                    const Pedido& p = todos[visibles[scroll + i]];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i, p.getFolio() == pedidoSel);
                    ventana.dibujarTexto(recortar(p.getFolio(), 13), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(p.getUsernameCliente(), 13), 410, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.getEstado(), 550, y, 18, colorEstado(p.getEstado()));
                    ventana.dibujarTexto(dinero(p.getTotal()), 740, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto(soloActivos ? "No hay pedidos activos por ahora."
                                                     : "Todavía no hay pedidos.",
                                         260, 150, 18, TEXTO_SUAVE);
                }

                // --- detalle del pedido seleccionado ---
                rectangulo(ventana, 250, 335, 620, 2, sf::Color(200, 190, 178));

                const Pedido* seleccionado = nullptr;
                for (const Pedido& p : todos) {
                    if (p.getFolio() == pedidoSel) { seleccionado = &p; break; }
                }

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Detalle del pedido", 260, 348, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto("Selecciona un pedido de la tabla para ver sus productos.",
                                         260, 378, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Pedido " + recortar(seleccionado->getFolio(), 30), 260, 345, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto("Cliente: " + recortar(seleccionado->getUsernameCliente(), 20) +
                                         "   ·   Fecha: " + seleccionado->getFecha(),
                                         260, 372, 15, TEXTO_SUAVE);

                    ventana.dibujarTexto("Estado:", 260, 394, 16, TEXTO_SUAVE);
                    ventana.dibujarTexto(seleccionado->getEstado(), 325, 394, 16, colorEstado(seleccionado->getEstado()));
                    ventana.dibujarTexto("Total: " + dinero(seleccionado->getTotal()), 560, 394, 16, TEXTO_OSCURO);

                    const vector<pair<Producto, int>>& detalle = cafeteria.getDetallePedido();
                    bool detalleVigente = (cafeteria.getFolioDetalle() == pedidoSel);
                    int maxLineas = 4;

                    if (!detalleVigente) {
                        ventana.dibujarTexto("Cargando productos...", 260, 420, 15, TEXTO_SUAVE);
                    }

                    for (int i = 0; detalleVigente && i < static_cast<int>(detalle.size()); i++) {
                        float y = 420.0f + i * 20.0f;

                        if (i == maxLineas - 1 && static_cast<int>(detalle.size()) > maxLineas) {
                            ventana.dibujarTexto("... y " + to_string(detalle.size() - i) + " productos más",
                                                 260, y, 15, TEXTO_SUAVE);
                            break;
                        }

                        const Producto& prod = detalle[i].first;
                        int cantidad = detalle[i].second;

                        ventana.dibujarTexto(to_string(cantidad) + " x " + recortar(prod.getNombreProducto(), 34),
                                             260, y, 15, TEXTO_OSCURO);
                        ventana.dibujarTexto(dinero(prod.getPrecio() * cantidad), 780, y, 15, TEXTO_SUAVE);
                    }

                    // --- botones de accion (solo los que tienen sentido para este estado) ---
                    string textoAvanzar, estadoSiguiente;
                    if (siguienteEstado(seleccionado->getEstado(), textoAvanzar, estadoSiguiente)) {
                        botonAvanzar.setTexto(textoAvanzar);
                        botonAvanzar.dibujar(ventana);
                    }

                    if (sePuedeCancelar(seleccionado->getEstado())) {
                        botonCancelar.dibujar(ventana);
                    }
                }
            }
            // ==================== VISTA INVENTARIO ====================
            else if (vista == Vista::INVENTARIO) {
                const vector<Producto> inv = cafeteria.getInventario();
                int total = static_cast<int>(inv.size());
                scrollMaximo = max(0, total - FILAS_INVENTARIO);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Inventario (" + to_string(total) + ")",
                                "Haz clic en un producto para reabastecerlo. Rueda del mouse para bajar.",
                                {{"ID", 260}, {"Producto", 340}, {"Precio", 640}, {"Stock", 760}});

                const Producto* seleccionado = nullptr;

                for (int i = 0; i < FILAS_INVENTARIO && scroll + i < total; i++) {
                    const Producto& p = inv[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;
                    bool esSel = (p.getIdProducto() == productoSel);

                    franja(ventana, i, esSel);
                    ventana.dibujarTexto(p.getIdProducto(), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(p.getNombreProducto(), 26), 340, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(dinero(p.getPrecio()), 640, y, 18, TEXTO_OSCURO);

                    if (p.getStock() == 0) {
                        ventana.dibujarTexto("Agotado", 760, y, 18, ROJO_ERROR);
                    } else if (p.getStock() <= 5) {
                        ventana.dibujarTexto(to_string(p.getStock()) + " (bajo)", 760, y, 18, ROJO_ERROR);
                    } else {
                        ventana.dibujarTexto(to_string(p.getStock()), 760, y, 18, TEXTO_OSCURO);
                    }
                }

                for (const Producto& p : inv) {
                    if (p.getIdProducto() == productoSel) { seleccionado = &p; break; }
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Esta cafetería todavía no tiene productos.", 260, 150, 18, TEXTO_SUAVE);
                }

                rectangulo(ventana, 250, 440, 620, 2, sf::Color(200, 190, 178));

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Selecciona un producto de la tabla.", 260, 452, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Seleccionado: " + recortar(seleccionado->getNombreProducto(), 30) +
                                         "   (stock actual: " + to_string(seleccionado->getStock()) + ")",
                                         260, 452, 16, TEXTO_OSCURO);
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
                                {{"ID", 260}, {"Producto", 340}, {"Precio", 640}, {"Stock", 760}});

                const Producto* seleccionado = nullptr;

                for (int i = 0; i < FILAS_CAJA && scroll + i < total; i++) {
                    const Producto& p = inv[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i, p.getIdProducto() == productoSel);
                    ventana.dibujarTexto(p.getIdProducto(), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(p.getNombreProducto(), 26), 340, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(dinero(p.getPrecio()), 640, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.getStock() == 0 ? "Agotado" : to_string(p.getStock()),
                                         760, y, 18, p.getStock() == 0 ? ROJO_ERROR : TEXTO_OSCURO);
                }

                for (const Producto& p : inv) {
                    if (p.getIdProducto() == productoSel) { seleccionado = &p; break; }
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Esta cafetería todavía no tiene productos.", 260, 150, 18, TEXTO_SUAVE);
                }

                rectangulo(ventana, 250, 385, 620, 2, sf::Color(200, 190, 178));

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Selecciona un producto de la tabla.", 260, 395, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Seleccionado: " + recortar(seleccionado->getNombreProducto(), 24) +
                                         "   " + dinero(seleccionado->getPrecio()) + " c/u",
                                         260, 395, 16, TEXTO_OSCURO);
                }

                campoCaja.dibujar(ventana);
                botonCobrar.dibujar(ventana);

                // ganancia del turno (a la derecha)
                ventana.dibujarTexto("Ganancia del turno", 640, 428, 15, TEXTO_SUAVE);
                ventana.dibujarTexto(dinero(cafeteria.getGananciaCajaTurno()), 640, 448, 30, VERDE);

                // ultimas ventas
                const vector<VentaCaja>& ventas = cafeteria.getHistorialCaja();
                ventana.dibujarTexto("Últimas ventas", 260, 505, 15, TEXTO_SUAVE);

                if (ventas.empty()) {
                    ventana.dibujarTexto("Todavía no hay ventas en este turno.", 260, 527, 15, TEXTO_SUAVE);
                } else {
                    for (int k = 0; k < 2 && k < static_cast<int>(ventas.size()); k++) {
                        const VentaCaja& v = ventas[ventas.size() - 1 - k];
                        ventana.dibujarTexto(to_string(v.cantidad) + " x " + recortar(v.nombreProducto, 30) +
                                             "  -  " + dinero(v.subtotal),
                                             260, 527.0f + k * 18.0f, 15, TEXTO_OSCURO);
                    }
                }
            }

            // --- mensaje de la ultima accion, o error de red (si lo hay) ---
            float yMensaje = (vista == Vista::PEDIDOS) ? 568.0f : 562.0f;

            if (!mensajeAccion.empty()) {
                ventana.dibujarTexto(mensajeAccion, 260, yMensaje, 15, accionOk ? VERDE : ROJO_ERROR);
            } else if (!mensajePanel.empty()) {
                ventana.dibujarTexto(mensajePanel, 260, yMensaje, 15, ROJO_ERROR);
            }
        }

        win.display();
    }

    // Al salir del ciclo, el destructor de "cafeteria" cierra el socket y el
    // servidor nos marca automaticamente como desconectados.
    return 0;
}
