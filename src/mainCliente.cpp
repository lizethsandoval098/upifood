// =====================================================================
//  UPIIFOOD - Cliente (interfaz grafica con SFML)
//
//  Misma idea que mainAdministrador.cpp: la VENTANA es la interfaz y todo
//  vive dentro de UN SOLO CICLO (eventos -> actualizar datos -> dibujar).
//  Nunca hay cin ni esperas largas con la ventana abierta.
//
//  Pantallas:  LOGIN  <->  REGISTRO  ->  PANEL
//  Vistas del PANEL:
//     CAFETERIAS -> MENU -> CARRITO -> PAGO      (hacer un pedido y pagarlo)
//     PEDIDOS  (historial + estado, se actualiza solo cada 3 s)
//     TARJETAS (ver y agregar tarjetas)
//
//  El cliente NUNCA abre la base de datos: todo lo pide al servidor.
// =====================================================================

#include "Cliente.h"
#include "Ventana.h"
#include "Widgets.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <stdexcept>

using namespace std;

enum class Pantalla { LOGIN, REGISTRO, PANEL };
enum class Vista { CAFETERIAS, MENU, CARRITO, PEDIDOS, TARJETAS, PAGO };

// ---- Colores (los mismos del administrador) ----
static const sf::Color FONDO(245, 241, 233);
static const sf::Color BARRA_LATERAL(74, 50, 35);
static const sf::Color CREMA(250, 246, 238);
static const sf::Color TEXTO_OSCURO(50, 40, 35);
static const sf::Color TEXTO_SUAVE(130, 120, 110);
static const sf::Color ROJO_ERROR(180, 40, 40);
static const sf::Color VERDE(40, 160, 90);
static const sf::Color AMBAR(200, 140, 30);
static const sf::Color AZUL(60, 110, 180);
static const sf::Color FILA_CLARA(255, 255, 255);
static const sf::Color FILA_OSCURA(246, 240, 230);
static const sf::Color FILA_SELECCION(226, 208, 186);

// ---- Medidas ----
static const int ANCHO = 900;
static const int ALTO = 600;
static const float TABLA_Y_FILAS = 140.0f;
static const float ALTO_FILA = 34.0f;
static const float SEGUNDOS_REFRESCO = 3.0f;

static string recortar(const string& s, size_t maximo) {
    if (s.size() <= maximo) return s;

    size_t corte = maximo;
    while (corte > 0 && (static_cast<unsigned char>(s[corte]) & 0xC0) == 0x80) {
        corte--;
    }

    return s.substr(0, corte) + "...";
}

static string dinero(float v) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "$%.2f", v);
    return buffer;
}

static void rectangulo(Ventana& v, float x, float y, float ancho, float alto, sf::Color color) {
    sf::RectangleShape r(sf::Vector2f(ancho, alto));
    r.setPosition(x, y);
    r.setFillColor(color);
    v.getWindow().draw(r);
}

// Boton chiquito para dentro de las filas (+, -, Quitar).
static void botonMini(Ventana& v, const sf::FloatRect& area, const string& texto, sf::Color color) {
    rectangulo(v, area.left, area.top, area.width, area.height, color);
    v.dibujarTextoCentrado(texto, area, 18, sf::Color::White);
}

static void encabezadoTabla(Ventana& v, const string& titulo, const string& subtitulo,
                            const vector<pair<string, float>>& columnas) {
    v.dibujarTexto(titulo, 260, 30, 26, TEXTO_OSCURO);
    v.dibujarTexto(subtitulo, 260, 66, 15, TEXTO_SUAVE);

    for (const auto& c : columnas) {
        v.dibujarTexto(c.first, c.second, 104, 16, TEXTO_SUAVE);
    }

    rectangulo(v, 250, 130, 620, 2, sf::Color(200, 190, 178));
}

static float filaY(int i) {
    return TABLA_Y_FILAS + i * ALTO_FILA;
}

static void franja(Ventana& v, int i, bool seleccionada = false) {
    sf::Color color = seleccionada ? FILA_SELECCION : ((i % 2 == 0) ? FILA_CLARA : FILA_OSCURA);
    rectangulo(v, 250, filaY(i) - 3, 620, ALTO_FILA - 2, color);
}

// Numero de fila (0, 1, 2...) que esta bajo el mouse, o -1 si no hay ninguna.
static int filaEn(float mx, float my, int filasVisibles) {
    if (mx < 250 || mx > 870) return -1;

    float relativo = my - (TABLA_Y_FILAS - 3);
    if (relativo < 0) return -1;

    int fila = static_cast<int>(relativo / ALTO_FILA);
    return (fila < filasVisibles) ? fila : -1;
}

// ---- Manejo de varios campos de texto (foco, Tab) ----
static void enfocar(vector<CampoTexto*>& campos, CampoTexto* objetivo) {
    for (CampoTexto* c : campos) c->setEnfocado(c == objetivo);
}

static void siguienteCampo(vector<CampoTexto*>& campos) {
    for (size_t i = 0; i < campos.size(); i++) {
        if (campos[i]->estaEnfocado()) {
            enfocar(campos, campos[(i + 1) % campos.size()]);
            return;
        }
    }

    if (!campos.empty()) enfocar(campos, campos[0]);
}

static void clicEnCampos(vector<CampoTexto*>& campos, float mx, float my) {
    for (CampoTexto* c : campos) {
        if (c->contiene(mx, my)) {
            enfocar(campos, c);
            return;
        }
    }
}

static void teclearEnCampos(vector<CampoTexto*>& campos, sf::Uint32 unicode) {
    for (CampoTexto* c : campos) c->recibirCaracter(unicode);
}

static sf::Color colorEstado(const string& estado) {
    if (estado == "Pendiente")  return AMBAR;
    if (estado == "Preparando") return AZUL;
    if (estado == "Listo")      return VERDE;
    if (estado == "Cancelado")  return ROJO_ERROR;
    return TEXTO_SUAVE; // Entregado u otro
}

int main(int argc, char* argv[]) {
    string ip = resolverIpServidor(argc, argv);

    Cliente cliente;
    cliente.setIpServidor(ip);

    // 1) Conectar ANTES de abrir la ventana (si falla, avisamos por consola).
    if (!cliente.conectar()) {
        cout << "No se pudo conectar al servidor. Revisa la IP/Tailscale y que 'servidor' este corriendo." << endl;
        return 1;
    }

    // 2) Abrir la ventana. Desde aqui el programa SOLO vive dentro del ciclo.
    Ventana ventana("UPIIFOOD - Cliente", ANCHO, ALTO);
    ventana.abrir();
    sf::RenderWindow& win = ventana.getWindow();

    // ---------------- LOGIN ----------------
    CampoTexto campoUsuario("Usuario", 300, 215, 300, 42);
    CampoTexto campoClave("Contraseña", 300, 295, 300, 42, true);
    Boton botonEntrar("Entrar", 300, 365, 300, 46);
    Boton botonCrearCuenta("Crear cuenta nueva", 300, 425, 300, 40);
    botonCrearCuenta.setColor(sf::Color(150, 130, 112));
    vector<CampoTexto*> camposLogin = {&campoUsuario, &campoClave};
    campoUsuario.setEnfocado(true);
    string mensajeLogin;
    bool mensajeLoginOk = false;

    // ---------------- REGISTRO ----------------
    CampoTexto regCorreo("Correo institucional (@alumno.ipn.mx)", 110, 170, 320, 42);
    CampoTexto regNombre("Nombre (sin apellidos)", 110, 245, 320, 42);
    CampoTexto regApellidoP("Apellido paterno", 110, 320, 320, 42);
    CampoTexto regApellidoM("Apellido materno", 110, 395, 320, 42);
    CampoTexto regAnio("Año en que ingresaste al IPN", 470, 170, 320, 42);
    CampoTexto regEscuela("Escuela (ej. UPIITA, ESCOM)", 470, 245, 320, 42);
    CampoTexto regClave("Contraseña (mínimo 8 caracteres)", 470, 320, 320, 42, true);
    Boton botonRegistrar("Registrarme", 470, 395, 320, 46);
    Boton botonVolver("Volver", 470, 455, 320, 40);
    botonVolver.setColor(sf::Color(150, 130, 112));
    regCorreo.setMaxLongitud(50);
    regAnio.setMaxLongitud(4);
    regAnio.setModo(ModoCampo::SOLO_DIGITOS);
    regEscuela.setMaxLongitud(12);
    vector<CampoTexto*> camposRegistro = {&regCorreo, &regNombre, &regApellidoP, &regApellidoM,
                                          &regAnio, &regEscuela, &regClave};
    string mensajeRegistro;

    // ---------------- PANEL: barra lateral ----------------
    Boton botonCafeterias("Cafeterías", 20, 110, 180, 44);
    Boton botonCarrito("Carrito", 20, 165, 180, 44);
    Boton botonPedidos("Mis pedidos", 20, 220, 180, 44);
    Boton botonTarjetas("Mis tarjetas", 20, 275, 180, 44);
    Boton botonSalir("Salir", 20, 535, 180, 44);

    // ---------------- PANEL: botones de las vistas ----------------
    Boton botonHacerPedido("Hacer pedido y pagar", 620, 516, 240, 44);
    Boton botonPagarPedido("Pagar este pedido", 660, 524, 210, 34); // en el detalle de Mis pedidos
    CampoTexto campoCvvPago("CVV de la tarjeta", 260, 380, 150, 42, true);
    campoCvvPago.setMaxLongitud(3);
    campoCvvPago.setModo(ModoCampo::SOLO_DIGITOS);
    vector<CampoTexto*> camposPago = {&campoCvvPago};
    Boton botonPagar("Pagar", 430, 380, 180, 42);
    Boton botonAgregarTarjetaDesdePago("Agregar tarjeta", 630, 380, 230, 42);
    botonAgregarTarjetaDesdePago.setColor(sf::Color(150, 130, 112));

    CampoTexto campoNumero("Número de tarjeta (16 dígitos)", 260, 360, 300, 42);
    CampoTexto campoCvvNuevo("CVV", 580, 360, 90, 42, true);
    CampoTexto campoVenc("Vence (MM/AA)", 690, 360, 170, 42);
    CampoTexto campoTitular("Nombre del titular", 260, 440, 300, 42);
    Boton botonGuardarTarjeta("Guardar tarjeta", 580, 440, 280, 42);
    campoNumero.setMaxLongitud(16);
    campoNumero.setModo(ModoCampo::SOLO_DIGITOS);
    campoCvvNuevo.setMaxLongitud(3);
    campoCvvNuevo.setModo(ModoCampo::SOLO_DIGITOS);
    campoVenc.setMaxLongitud(5);
    campoVenc.setModo(ModoCampo::FECHA_MMAA);
    vector<CampoTexto*> camposTarjeta = {&campoNumero, &campoCvvNuevo, &campoVenc, &campoTitular};

    // ---------------- Estado del programa ----------------
    Pantalla pantalla = Pantalla::LOGIN;
    Vista vista = Vista::CAFETERIAS;
    int scroll = 0;
    int scrollMaximo = 0;
    string aviso;               // mensaje de abajo (errores / confirmaciones)
    bool avisoOk = false;       // true = verde, false = rojo
    bool avisoDeRed = false;    // errores de refresco: se borran solos cuando vuelve la red
    sf::Clock relojRefresco;

    string idCafeteriaSel;      // cafeteria cuyo menu se esta viendo
    string folioPago;           // pedido que se esta pagando
    float totalPago = 0.0f;
    int tarjetaSel = -1;
    string pedidoSel;           // folio del pedido seleccionado en "Mis pedidos" (para ver su detalle)

    auto filasVisibles = [&]() -> int {
        if (vista == Vista::PEDIDOS) return 5; // abajo va el panel con el detalle del pedido
        return (vista == Vista::TARJETAS || vista == Vista::PAGO) ? 4 : 10;
    };

    // Resumen del pedido seleccionado (nullptr si no hay o ya no existe).
    auto pedidoSeleccionado = [&]() -> const ResumenPedidoCliente* {
        if (pedidoSel.empty()) return nullptr;

        for (const ResumenPedidoCliente& p : cliente.getPedidosCliente()) {
            if (p.folio == pedidoSel) return &p;
        }

        return nullptr;
    };

    auto totalFilas = [&]() -> int {
        switch (vista) {
            case Vista::CAFETERIAS: return static_cast<int>(cliente.getCafeterias().size());
            case Vista::MENU:       return static_cast<int>(cliente.getMenu().size());
            case Vista::CARRITO:    return static_cast<int>(cliente.getCarrito().size());
            case Vista::PEDIDOS:    return static_cast<int>(cliente.getPedidosCliente().size());
            case Vista::TARJETAS:
            case Vista::PAGO:       return static_cast<int>(cliente.getTarjetasRed().size());
        }
        return 0;
    };

    auto ajustarScroll = [&]() {
        scrollMaximo = max(0, totalFilas() - filasVisibles());
        scroll = max(0, min(scroll, scrollMaximo));
    };

    auto ponerAviso = [&](const string& texto, bool ok, bool deRed = false) {
        aviso = texto;
        avisoOk = ok;
        avisoDeRed = deRed;
    };

    // Pide al servidor los datos de la vista actual (sin tocar el aviso si todo sale bien).
    auto cargarVista = [&]() {
        bool ok = true;

        switch (vista) {
            case Vista::CAFETERIAS: ok = cliente.cargarCafeterias(); break;
            case Vista::MENU:       ok = cliente.cargarMenu(idCafeteriaSel); break;
            case Vista::PEDIDOS:
                ok = cliente.cargarPedidosCliente();

                if (ok && !pedidoSel.empty()) {
                    const ResumenPedidoCliente* sel = pedidoSeleccionado();

                    if (sel == nullptr) {
                        pedidoSel.clear();                                       // ya no existe
                    } else {
                        cliente.cargarDetallePedido(sel->folio, sel->idCafeteria); // refresca sus productos
                    }
                }
                break;
            case Vista::TARJETAS:   ok = cliente.cargarTarjetasRed(); break;
            case Vista::PAGO:       ok = cliente.cargarTarjetasRed(); break;
            case Vista::CARRITO:    break; // el carrito es local
        }

        if (!ok) {
            ponerAviso("Sin datos: " + cliente.getUltimoError(), false, true);
        } else if (avisoDeRed) {
            aviso.clear();
            avisoDeRed = false;
        }

        if (vista == Vista::PAGO && ok) {
            const int n = static_cast<int>(cliente.getTarjetasRed().size());
            if (tarjetaSel >= n) tarjetaSel = -1;
            if (tarjetaSel < 0 && n > 0) tarjetaSel = 0;
        }

        relojRefresco.restart();
    };

    auto cambiarVista = [&](Vista nueva) {
        vista = nueva;
        scroll = 0;
        aviso.clear();
        avisoDeRed = false;
        cargarVista();
    };

    auto intentarLogin = [&]() {
        if (cliente.iniciarSesion(campoUsuario.getContenido(), campoClave.getContenido())) {
            pantalla = Pantalla::PANEL;
            mensajeLogin.clear();
            campoClave.limpiar();
            cliente.cargarCafeterias(); // para mostrar nombres de cafeterias en los pedidos
            cambiarVista(Vista::CAFETERIAS);
        } else {
            mensajeLogin = cliente.getUltimoError();
            mensajeLoginOk = false;
            campoClave.limpiar();
        }
    };

    auto intentarRegistro = [&]() {
        string usernameNuevo;

        if (cliente.registrarse(regCorreo.getContenido(), regNombre.getContenido(),
                                regApellidoP.getContenido(), regApellidoM.getContenido(),
                                regAnio.getContenido(), regEscuela.getContenido(),
                                regClave.getContenido(), usernameNuevo)) {
            for (CampoTexto* c : camposRegistro) c->limpiar();
            mensajeRegistro.clear();

            campoUsuario.setContenido(usernameNuevo);
            campoClave.limpiar();
            enfocar(camposLogin, &campoClave);
            mensajeLogin = "¡Cuenta creada! Tu usuario es: " + usernameNuevo;
            mensajeLoginOk = true;
            pantalla = Pantalla::LOGIN;
        } else {
            mensajeRegistro = cliente.getUltimoError();
        }
    };

    auto intentarPagar = [&]() {
        const vector<InfoTarjeta>& tarjetas = cliente.getTarjetasRed();

        if (folioPago.empty()) {
            ponerAviso("No hay ningún pedido por pagar.", false);
            return;
        }
        if (tarjetaSel < 0 || tarjetaSel >= static_cast<int>(tarjetas.size())) {
            ponerAviso("Elige una tarjeta (o agrega una en 'Mis tarjetas').", false);
            return;
        }
        if (campoCvvPago.getContenido().empty()) {
            ponerAviso("Escribe el CVV de la tarjeta.", false);
            return;
        }

        string referencia;

        if (cliente.pagarPedidoRed(folioPago, tarjetas[tarjetaSel].numero,
                                   campoCvvPago.getContenido(), referencia)) {
            campoCvvPago.limpiar();
            folioPago.clear();
            cambiarVista(Vista::PEDIDOS);
            ponerAviso("Pago aprobado. Referencia: " + referencia, true);
        } else {
            campoCvvPago.limpiar();
            ponerAviso(cliente.getUltimoError(), false);
        }
    };

    auto intentarGuardarTarjeta = [&]() {
        if (cliente.agregarTarjetaRed(campoNumero.getContenido(), campoCvvNuevo.getContenido(),
                                      campoTitular.getContenido(), campoVenc.getContenido())) {
            for (CampoTexto* c : camposTarjeta) c->limpiar();
            cargarVista();
            ponerAviso("Tarjeta guardada.", true);
        } else {
            ponerAviso(cliente.getUltimoError(), false);
        }
    };

    auto hacerPedido = [&]() {
        string folio;
        float total = 0.0f;

        if (cliente.crearPedidoDesdeCarrito(folio, total)) {
            folioPago = folio;
            totalPago = total;
            tarjetaSel = -1;
            campoCvvPago.limpiar();
            enfocar(camposPago, &campoCvvPago);
            cambiarVista(Vista::PAGO);
            ponerAviso("Pedido creado. Falta pagarlo para que la cafetería lo prepare.", true);
        } else {
            ponerAviso(cliente.getUltimoError(), false);
        }
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

            // ======================= LOGIN =======================
            if (pantalla == Pantalla::LOGIN) {

                if (e.type == sf::Event::MouseMoved) {
                    float mx = static_cast<float>(e.mouseMove.x);
                    float my = static_cast<float>(e.mouseMove.y);
                    botonEntrar.actualizarHover(mx, my);
                    botonCrearCuenta.actualizarHover(mx, my);
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    clicEnCampos(camposLogin, mx, my);

                    if (botonEntrar.contiene(mx, my)) {
                        intentarLogin();
                    }
                    else if (botonCrearCuenta.contiene(mx, my)) {
                        mensajeLogin.clear();
                        mensajeRegistro.clear();
                        enfocar(camposRegistro, &regCorreo);
                        pantalla = Pantalla::REGISTRO;
                    }
                }
                else if (e.type == sf::Event::TextEntered) {
                    teclearEnCampos(camposLogin, e.text.unicode);
                }
                else if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Tab)         siguienteCampo(camposLogin);
                    else if (e.key.code == sf::Keyboard::Return) intentarLogin();
                }
            }
            // ====================== REGISTRO ======================
            else if (pantalla == Pantalla::REGISTRO) {

                if (e.type == sf::Event::MouseMoved) {
                    float mx = static_cast<float>(e.mouseMove.x);
                    float my = static_cast<float>(e.mouseMove.y);
                    botonRegistrar.actualizarHover(mx, my);
                    botonVolver.actualizarHover(mx, my);
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    clicEnCampos(camposRegistro, mx, my);

                    if (botonRegistrar.contiene(mx, my)) {
                        intentarRegistro();
                    }
                    else if (botonVolver.contiene(mx, my)) {
                        mensajeRegistro.clear();
                        pantalla = Pantalla::LOGIN;
                    }
                }
                else if (e.type == sf::Event::TextEntered) {
                    teclearEnCampos(camposRegistro, e.text.unicode);
                }
                else if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Tab)         siguienteCampo(camposRegistro);
                    else if (e.key.code == sf::Keyboard::Return) intentarRegistro();
                }
            }
            // ======================= PANEL =======================
            else {

                if (e.type == sf::Event::MouseMoved) {
                    float mx = static_cast<float>(e.mouseMove.x);
                    float my = static_cast<float>(e.mouseMove.y);

                    botonCafeterias.actualizarHover(mx, my);
                    botonCarrito.actualizarHover(mx, my);
                    botonPedidos.actualizarHover(mx, my);
                    botonTarjetas.actualizarHover(mx, my);
                    botonSalir.actualizarHover(mx, my);
                    botonHacerPedido.actualizarHover(mx, my);
                    botonPagarPedido.actualizarHover(mx, my);
                    botonPagar.actualizarHover(mx, my);
                    botonAgregarTarjetaDesdePago.actualizarHover(mx, my);
                    botonGuardarTarjeta.actualizarHover(mx, my);
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    // ---- barra lateral ----
                    if (botonCafeterias.contiene(mx, my))      cambiarVista(Vista::CAFETERIAS);
                    else if (botonCarrito.contiene(mx, my))    cambiarVista(Vista::CARRITO);
                    else if (botonPedidos.contiene(mx, my))    cambiarVista(Vista::PEDIDOS);
                    else if (botonTarjetas.contiene(mx, my)) {
                        enfocar(camposTarjeta, &campoNumero);
                        cambiarVista(Vista::TARJETAS);
                    }
                    else if (botonSalir.contiene(mx, my))      ventana.cerrar();

                    // ---- contenido de cada vista ----
                    else if (vista == Vista::CAFETERIAS) {
                        int fila = filaEn(mx, my, filasVisibles());
                        int idx = scroll + fila;

                        if (fila >= 0 && idx < totalFilas()) {
                            idCafeteriaSel = cliente.getCafeterias()[idx].id;
                            cambiarVista(Vista::MENU);
                        }
                    }
                    else if (vista == Vista::MENU) {
                        int fila = filaEn(mx, my, filasVisibles());
                        int idx = scroll + fila;

                        if (fila >= 0 && idx < totalFilas()) {
                            const Producto producto = cliente.getMenu()[idx];
                            float y = filaY(fila);

                            if (sf::FloatRect(830, y - 2, 30, 26).contains(mx, my)) {
                                bool cambiaDeCafeteria = !cliente.getCarrito().empty() &&
                                                         cliente.getIdCafeteriaCarrito() != idCafeteriaSel;

                                if (cliente.agregarAlCarrito(idCafeteriaSel, producto)) {
                                    if (cambiaDeCafeteria) {
                                        ponerAviso("Tu carrito anterior se vació: un pedido es de una sola cafetería.", true);
                                    } else {
                                        aviso.clear();
                                    }
                                } else {
                                    ponerAviso(cliente.getUltimoError(), false);
                                }
                            }
                            else if (sf::FloatRect(790, y - 2, 30, 26).contains(mx, my)) {
                                cliente.quitarUnoDelCarrito(producto.getIdProducto());
                                aviso.clear();
                            }
                        }
                    }
                    else if (vista == Vista::CARRITO) {
                        if (botonHacerPedido.contiene(mx, my)) {
                            hacerPedido();
                        } else {
                            int fila = filaEn(mx, my, filasVisibles());
                            int idx = scroll + fila;

                            if (fila >= 0 && idx < totalFilas() &&
                                sf::FloatRect(800, filaY(fila) - 2, 60, 26).contains(mx, my)) {
                                cliente.quitarDelCarrito(cliente.getCarrito()[idx].first.getIdProducto());
                                aviso.clear();
                            }
                        }
                    }
                    else if (vista == Vista::PEDIDOS) {
                        const ResumenPedidoCliente* sel = pedidoSeleccionado();

                        // Boton "Pagar este pedido" (solo aparece si esta sin pagar y no cancelado).
                        if (sel != nullptr && !sel->pagado && sel->estado != "Cancelado" &&
                            botonPagarPedido.contiene(mx, my)) {
                            folioPago = sel->folio;
                            totalPago = sel->total;
                            tarjetaSel = -1;
                            campoCvvPago.limpiar();
                            enfocar(camposPago, &campoCvvPago);
                            cambiarVista(Vista::PAGO);
                        } else {
                            int fila = filaEn(mx, my, filasVisibles());
                            int idx = scroll + fila;

                            if (fila >= 0 && idx < totalFilas()) {
                                const ResumenPedidoCliente& p = cliente.getPedidosCliente()[idx];

                                pedidoSel = p.folio;
                                cliente.cargarDetallePedido(p.folio, p.idCafeteria);
                                aviso.clear();
                                avisoDeRed = false;
                            }
                        }
                    }
                    else if (vista == Vista::TARJETAS) {
                        clicEnCampos(camposTarjeta, mx, my);

                        if (botonGuardarTarjeta.contiene(mx, my)) intentarGuardarTarjeta();
                    }
                    else if (vista == Vista::PAGO) {
                        clicEnCampos(camposPago, mx, my);

                        int fila = filaEn(mx, my, filasVisibles());
                        int idx = scroll + fila;

                        if (fila >= 0 && idx < totalFilas()) {
                            tarjetaSel = idx;
                        }
                        else if (botonPagar.contiene(mx, my)) {
                            intentarPagar();
                        }
                        else if (botonAgregarTarjetaDesdePago.contiene(mx, my)) {
                            enfocar(camposTarjeta, &campoNumero);
                            cambiarVista(Vista::TARJETAS);
                        }
                    }
                }
                else if (e.type == sf::Event::MouseWheelScrolled) {
                    scroll -= static_cast<int>(e.mouseWheelScroll.delta);
                    ajustarScroll();
                }
                else if (e.type == sf::Event::TextEntered) {
                    if (vista == Vista::TARJETAS) teclearEnCampos(camposTarjeta, e.text.unicode);
                    else if (vista == Vista::PAGO) teclearEnCampos(camposPago, e.text.unicode);
                }
                else if (e.type == sf::Event::KeyPressed) {
                    if (vista == Vista::TARJETAS) {
                        if (e.key.code == sf::Keyboard::Tab)         siguienteCampo(camposTarjeta);
                        else if (e.key.code == sf::Keyboard::Return) intentarGuardarTarjeta();
                    }
                    else if (vista == Vista::PAGO && e.key.code == sf::Keyboard::Return) {
                        intentarPagar();
                    }
                }
            }
        }

        if (!ventana.estaAbierta()) {
            break;
        }

        // ---------- 2) ACTUALIZAR DATOS (cada 3 segundos) ----------
        // Asi el cliente ve casi en tiempo real como la cafeteria cambia el
        // estado de su pedido (Pendiente -> Preparando -> Listo -> Entregado).
        if (pantalla == Pantalla::PANEL &&
            relojRefresco.getElapsedTime().asSeconds() >= SEGUNDOS_REFRESCO) {
            if (vista == Vista::PEDIDOS || vista == Vista::MENU || vista == Vista::CAFETERIAS) {
                cargarVista();
            } else {
                relojRefresco.restart();
            }
        }

        // ---------- 3) DIBUJAR ----------
        win.clear(FONDO);

        if (pantalla == Pantalla::LOGIN) {

            ventana.dibujarTextoCentrado("UPIIFOOD", sf::FloatRect(0, 70, ANCHO, 60), 52, BARRA_LATERAL);
            ventana.dibujarTextoCentrado("Acceso de clientes", sf::FloatRect(0, 135, ANCHO, 30), 22, TEXTO_SUAVE);

            campoUsuario.dibujar(ventana);
            campoClave.dibujar(ventana);
            botonEntrar.dibujar(ventana);
            botonCrearCuenta.dibujar(ventana);

            if (!mensajeLogin.empty()) {
                ventana.dibujarTextoCentrado(mensajeLogin, sf::FloatRect(0, 480, ANCHO, 30), 18,
                                             mensajeLoginOk ? VERDE : ROJO_ERROR);
            }

            ventana.dibujarTextoCentrado("Servidor: " + ip, sf::FloatRect(0, 560, ANCHO, 24), 14, TEXTO_SUAVE);
        }
        else if (pantalla == Pantalla::REGISTRO) {

            ventana.dibujarTextoCentrado("Crear cuenta", sf::FloatRect(0, 30, ANCHO, 50), 40, BARRA_LATERAL);
            ventana.dibujarTextoCentrado("Solo para alumnos del IPN (correo @alumno.ipn.mx)",
                                         sf::FloatRect(0, 85, ANCHO, 30), 18, TEXTO_SUAVE);
            ventana.dibujarTextoCentrado("Correo: inicial del nombre + apellido paterno + inicial del materno + 4 dígitos (los 2 primeros = año de ingreso)",
                                         sf::FloatRect(0, 112, ANCHO, 24), 14, TEXTO_SUAVE);

            for (CampoTexto* c : camposRegistro) c->dibujar(ventana);
            botonRegistrar.dibujar(ventana);
            botonVolver.dibujar(ventana);

            ventana.dibujarTexto("Tu usuario será lo que va antes de la @ de tu correo.", 110, 460, 15, TEXTO_SUAVE);

            if (!mensajeRegistro.empty()) {
                ventana.dibujarTextoCentrado(mensajeRegistro, sf::FloatRect(0, 530, ANCHO, 30), 17, ROJO_ERROR);
            }
        }
        else {
            // --- barra lateral ---
            rectangulo(ventana, 0, 0, 220, ALTO, BARRA_LATERAL);
            ventana.dibujarTexto("UPIIFOOD", 20, 25, 26, CREMA);
            ventana.dibujarTexto("Cliente", 20, 62, 16, sf::Color(200, 180, 160));

            botonCarrito.setTexto("Carrito (" + to_string(cliente.unidadesEnCarrito()) + ")");

            botonCafeterias.setActivo(vista == Vista::CAFETERIAS || vista == Vista::MENU);
            botonCarrito.setActivo(vista == Vista::CARRITO);
            botonPedidos.setActivo(vista == Vista::PEDIDOS || vista == Vista::PAGO);
            botonTarjetas.setActivo(vista == Vista::TARJETAS);

            botonCafeterias.dibujar(ventana);
            botonCarrito.dibujar(ventana);
            botonPedidos.dibujar(ventana);
            botonTarjetas.dibujar(ventana);
            botonSalir.dibujar(ventana);

            ventana.dibujarTexto("Sesión de:", 20, 440, 13, sf::Color(200, 180, 160));
            ventana.dibujarTexto(recortar(cliente.getNombre(), 18), 20, 460, 16, CREMA);
            ventana.dibujarTexto("@" + recortar(cliente.getUsername(), 17), 20, 484, 14, sf::Color(200, 180, 160));
            ventana.dibujarTexto(cliente.getTipoCliente(), 20, 506, 14, sf::Color(200, 180, 160));

            // --- zona de contenido ---
            rectangulo(ventana, 240, 15, 650, 570, sf::Color(255, 255, 255));
            ajustarScroll();

            const int visibles = filasVisibles();

            if (vista == Vista::CAFETERIAS) {
                const vector<InfoCafeteria>& lista = cliente.getCafeterias();
                int total = static_cast<int>(lista.size());

                encabezadoTabla(ventana, "Cafeterías (" + to_string(total) + ")",
                                "Haz clic en una cafetería para ver su menú.",
                                {{"#", 260}, {"Cafetería", 310}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    float y = filaY(i);
                    franja(ventana, i);
                    ventana.dibujarTexto(to_string(scroll + i + 1), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(lista[scroll + i].nombre, 40), 310, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto("Ver menú >", 760, y, 16, TEXTO_SUAVE);
                }

                if (total == 0 && aviso.empty()) {
                    ventana.dibujarTexto("Todavía no hay cafeterías registradas.", 260, 150, 18, TEXTO_SUAVE);
                }
            }
            else if (vista == Vista::MENU) {
                const vector<Producto>& lista = cliente.getMenu();
                int total = static_cast<int>(lista.size());

                encabezadoTabla(ventana, "Menú: " + recortar(cliente.nombreCafeteria(idCafeteriaSel), 26),
                                "Usa + y - para armar tu pedido. El stock se actualiza solo.",
                                {{"Producto", 260}, {"Precio", 510}, {"Stock", 600}, {"En carrito", 680}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    const Producto& p = lista[scroll + i];
                    float y = filaY(i);
                    int enCarrito = (cliente.getIdCafeteriaCarrito() == idCafeteriaSel)
                                        ? cliente.cantidadEnCarrito(p.getIdProducto()) : 0;

                    franja(ventana, i);
                    ventana.dibujarTexto(recortar(p.getNombreProducto(), 24), 260, y, 18,
                                         p.getStock() > 0 ? TEXTO_OSCURO : TEXTO_SUAVE);
                    ventana.dibujarTexto(dinero(p.getPrecio()), 510, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.getStock() > 0 ? to_string(p.getStock()) : "Agotado", 600, y, 18,
                                         p.getStock() > 0 ? TEXTO_OSCURO : ROJO_ERROR);
                    ventana.dibujarTexto(to_string(enCarrito), 700, y, 18, TEXTO_OSCURO);

                    botonMini(ventana, sf::FloatRect(790, y - 2, 30, 26), "-", sf::Color(150, 130, 112));
                    botonMini(ventana, sf::FloatRect(830, y - 2, 30, 26), "+",
                              p.getStock() > 0 ? VERDE : sf::Color(190, 180, 170));
                }

                if (total == 0 && aviso.empty()) {
                    ventana.dibujarTexto("Esta cafetería todavía no tiene productos.", 260, 150, 18, TEXTO_SUAVE);
                }

                ventana.dibujarTexto("En tu carrito: " + to_string(cliente.unidadesEnCarrito()) +
                                     " producto(s) - " + dinero(cliente.totalCarrito()),
                                     260, 522, 18, TEXTO_OSCURO);
            }
            else if (vista == Vista::CARRITO) {
                const vector<pair<Producto, int>>& lista = cliente.getCarrito();
                int total = static_cast<int>(lista.size());

                string subtitulo = lista.empty()
                    ? "Tu carrito está vacío. Elige productos en 'Cafeterías'."
                    : "Pedido para: " + cliente.nombreCafeteria(cliente.getIdCafeteriaCarrito());

                encabezadoTabla(ventana, "Tu carrito", subtitulo,
                                {{"Producto", 260}, {"Cantidad", 500}, {"Subtotal", 670}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    const Producto& p = lista[scroll + i].first;
                    int cantidad = lista[scroll + i].second;
                    float y = filaY(i);

                    franja(ventana, i);
                    ventana.dibujarTexto(recortar(p.getNombreProducto(), 26), 260, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(to_string(cantidad) + " x " + dinero(p.getPrecio()), 500, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(dinero(p.getPrecio() * cantidad), 670, y, 18, TEXTO_OSCURO);
                    botonMini(ventana, sf::FloatRect(800, y - 2, 60, 26), "Quitar", ROJO_ERROR);
                }

                ventana.dibujarTexto("Total: " + dinero(cliente.totalCarrito()), 260, 522, 24, TEXTO_OSCURO);
                botonHacerPedido.dibujar(ventana);
            }
            else if (vista == Vista::PEDIDOS) {
                const vector<ResumenPedidoCliente>& lista = cliente.getPedidosCliente();
                int total = static_cast<int>(lista.size());

                encabezadoTabla(ventana, "Mis pedidos (" + to_string(total) + ")",
                                "Se actualiza solo. Haz clic en un pedido para ver su detalle.",
                                {{"Folio", 260}, {"Cafetería", 420}, {"Estado", 560}, {"Total", 660}, {"Pago", 750}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    const ResumenPedidoCliente& p = lista[scroll + i];
                    float y = filaY(i);

                    franja(ventana, i, p.folio == pedidoSel);
                    ventana.dibujarTexto(recortar(p.folio, 16), 260, y, 16, TEXTO_OSCURO);
                    ventana.dibujarTexto(recortar(cliente.nombreCafeteria(p.idCafeteria), 12), 420, y, 16, TEXTO_OSCURO);
                    ventana.dibujarTexto(p.estado, 560, y, 16, colorEstado(p.estado));
                    ventana.dibujarTexto(dinero(p.total), 660, y, 16, TEXTO_OSCURO);

                    if (p.pagado) {
                        ventana.dibujarTexto("Pagado", 750, y, 16, VERDE);
                    } else if (p.estado == "Cancelado") {
                        ventana.dibujarTexto("-", 750, y, 16, TEXTO_SUAVE);
                    } else {
                        ventana.dibujarTexto("Por pagar", 750, y, 16, ROJO_ERROR);
                    }
                }

                if (total == 0 && aviso.empty()) {
                    ventana.dibujarTexto("Todavía no has hecho pedidos.", 260, 150, 18, TEXTO_SUAVE);
                }

                // --- detalle del pedido seleccionado (igual que en la cafetería) ---
                rectangulo(ventana, 250, 318, 620, 2, sf::Color(200, 190, 178));

                const ResumenPedidoCliente* seleccionado = pedidoSeleccionado();

                if (seleccionado == nullptr) {
                    ventana.dibujarTexto("Detalle del pedido", 260, 330, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto("Selecciona un pedido de la tabla para ver sus productos.",
                                         260, 360, 16, TEXTO_SUAVE);
                } else {
                    ventana.dibujarTexto("Pedido " + recortar(seleccionado->folio, 30), 260, 328, 20, TEXTO_OSCURO);
                    ventana.dibujarTexto("Cafetería: " + recortar(cliente.nombreCafeteria(seleccionado->idCafeteria), 24) +
                                         "   ·   Fecha: " + seleccionado->fecha,
                                         260, 358, 15, TEXTO_SUAVE);

                    ventana.dibujarTexto("Estado:", 260, 384, 17, TEXTO_SUAVE);
                    ventana.dibujarTexto(seleccionado->estado, 335, 384, 17, colorEstado(seleccionado->estado));

                    ventana.dibujarTexto("Pago:", 450, 384, 17, TEXTO_SUAVE);
                    ventana.dibujarTexto(seleccionado->pagado ? "Pagado" : "No pagado", 500, 384, 17,
                                         seleccionado->pagado ? VERDE : ROJO_ERROR);

                    ventana.dibujarTexto("Total: " + dinero(seleccionado->total), 660, 384, 17, TEXTO_OSCURO);

                    const vector<pair<Producto, int>>& detalle = cliente.getDetallePedido();
                    bool detalleVigente = (cliente.getFolioDetalle() == pedidoSel);
                    int maxLineas = 5;

                    if (!detalleVigente) {
                        ventana.dibujarTexto("Cargando productos...", 260, 414, 16, TEXTO_SUAVE);
                    }

                    for (int i = 0; detalleVigente && i < static_cast<int>(detalle.size()); i++) {
                        float y = 414.0f + i * 20.0f;

                        if (i == maxLineas - 1 && static_cast<int>(detalle.size()) > maxLineas) {
                            ventana.dibujarTexto("... y " + to_string(detalle.size() - i) + " productos más",
                                                 260, y, 16, TEXTO_SUAVE);
                            break;
                        }

                        const Producto& prod = detalle[i].first;
                        int cantidad = detalle[i].second;

                        ventana.dibujarTexto(to_string(cantidad) + " x " + recortar(prod.getNombreProducto(), 40),
                                             260, y, 16, TEXTO_OSCURO);
                        ventana.dibujarTexto(dinero(prod.getPrecio() * cantidad), 780, y, 16, TEXTO_SUAVE);
                    }

                    // Si todavia no se paga, desde aqui se puede pagar.
                    if (!seleccionado->pagado && seleccionado->estado != "Cancelado") {
                        botonPagarPedido.dibujar(ventana);
                    }
                }
            }
            else if (vista == Vista::TARJETAS) {
                const vector<InfoTarjeta>& lista = cliente.getTarjetasRed();
                int total = static_cast<int>(lista.size());

                encabezadoTabla(ventana, "Mis tarjetas (" + to_string(total) + ")",
                                "Se usan para pagar tus pedidos.",
                                {{"Tarjeta", 260}, {"Vence", 520}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    float y = filaY(i);
                    franja(ventana, i);
                    ventana.dibujarTexto(lista[scroll + i].enmascarada, 260, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(lista[scroll + i].vencimiento, 520, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && aviso.empty()) {
                    ventana.dibujarTexto("Aún no tienes tarjetas guardadas.", 260, 150, 18, TEXTO_SUAVE);
                }

                ventana.dibujarTexto("Agregar tarjeta nueva", 260, 296, 20, TEXTO_OSCURO);

                campoNumero.dibujar(ventana);
                campoCvvNuevo.dibujar(ventana);
                campoVenc.dibujar(ventana);
                campoTitular.dibujar(ventana);
                botonGuardarTarjeta.dibujar(ventana);
            }
            else { // PAGO
                const vector<InfoTarjeta>& lista = cliente.getTarjetasRed();
                int total = static_cast<int>(lista.size());

                encabezadoTabla(ventana, "Pagar pedido",
                                "Folio " + folioPago + "  -  Total " + dinero(totalPago),
                                {{"Elige la tarjeta", 260}, {"Vence", 520}});

                for (int i = 0; i < visibles && scroll + i < total; i++) {
                    float y = filaY(i);
                    bool elegida = (scroll + i == tarjetaSel);

                    franja(ventana, i, elegida);
                    ventana.dibujarTexto(lista[scroll + i].enmascarada, 260, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(lista[scroll + i].vencimiento, 520, y, 18, TEXTO_OSCURO);

                    if (elegida) {
                        ventana.dibujarTexto("Seleccionada", 740, y, 16, VERDE);
                    }
                }

                if (total == 0) {
                    ventana.dibujarTexto("No tienes tarjetas: agrega una con el botón de abajo.", 260, 150, 18, TEXTO_SUAVE);
                }

                campoCvvPago.dibujar(ventana);
                botonPagar.dibujar(ventana);
                botonAgregarTarjetaDesdePago.dibujar(ventana);

                ventana.dibujarTexto("Se cobrará el total del pedido a la tarjeta elegida.", 260, 450, 15, TEXTO_SUAVE);
            }

            // --- aviso (errores / confirmaciones) ---
            if (!aviso.empty()) {
                ventana.dibujarTexto(recortar(aviso, 80), 260, 566, 15, avisoOk ? VERDE : ROJO_ERROR);
            }
        }

        win.display();
    }

    // Al salir del ciclo, el destructor de "cliente" cierra el socket y el
    // servidor nos marca automaticamente como desconectados.
    return 0;
}
