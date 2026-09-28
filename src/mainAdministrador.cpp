// =====================================================================
//  UPIIFOOD - Administrador (interfaz grafica con SFML)
//
//  IDEA CLAVE (esto es lo que contesta "como se hace lo de SFML junto con
//  todo lo demas"):
//
//    La VENTANA ES la interfaz. Ya no hay menu en la consola: todo se hace
//    con clics y teclado dentro de la ventana, en UN SOLO CICLO que se
//    repite ~60 veces por segundo:
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
// =====================================================================

#include "Administrador.h"
#include "Ventana.h"
#include "Widgets.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

enum class Pantalla { LOGIN, PANEL };
enum class Vista { USUARIOS, CAFETERIAS, EN_LINEA };

// ---- Colores ----
static const sf::Color FONDO(245, 241, 233);
static const sf::Color BARRA_LATERAL(74, 50, 35);
static const sf::Color CREMA(250, 246, 238);
static const sf::Color TEXTO_OSCURO(50, 40, 35);
static const sf::Color TEXTO_SUAVE(130, 120, 110);
static const sf::Color ROJO_ERROR(180, 40, 40);
static const sf::Color VERDE(40, 160, 90);
static const sf::Color FILA_CLARA(255, 255, 255);
static const sf::Color FILA_OSCURA(246, 240, 230);

// ---- Medidas de la ventana y de la zona de tablas ----
static const int ANCHO = 900;
static const int ALTO = 600;
static const float TABLA_Y_FILAS = 140.0f;
static const float ALTO_FILA = 30.0f;
static const int FILAS_VISIBLES = 14;
static const float SEGUNDOS_REFRESCO = 3.0f;

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

// Franja de fondo de la fila i (alternando color para que se lea mejor).
static void franja(Ventana& v, int i) {
    float y = TABLA_Y_FILAS + i * ALTO_FILA;
    rectangulo(v, 250, y - 3, 620, ALTO_FILA - 2, (i % 2 == 0) ? FILA_CLARA : FILA_OSCURA);
}

int main(int argc, char* argv[]) {
    string ip = resolverIpServidor(argc, argv);

    Administrador admin;
    admin.setIpServidor(ip);

    // 1) Conectar al servidor ANTES de abrir la ventana (si falla, avisamos por consola).
    if (!admin.conectar()) {
        return 1;
    }

    // 2) Ahora si: abrir la ventana. Desde aqui el programa SOLO vive dentro del ciclo.
    Ventana ventana("UPIIFOOD - Administrador", ANCHO, ALTO);
    ventana.abrir();
    sf::RenderWindow& win = ventana.getWindow();

    // ---------------- Componentes de la pantalla LOGIN ----------------
    CampoTexto campoUsuario("Usuario", 300, 215, 300, 42);
    CampoTexto campoClave("Contraseña", 300, 295, 300, 42, true);
    Boton botonEntrar("Entrar", 300, 365, 300, 46);
    campoUsuario.setEnfocado(true); // el cursor empieza en "Usuario"
    string mensajeLogin;

    // ---------------- Componentes de la pantalla PANEL ----------------
    Boton botonUsuarios("Usuarios", 20, 110, 180, 44);
    Boton botonCafeterias("Cafeterías", 20, 165, 180, 44);
    Boton botonEnLinea("En línea", 20, 220, 180, 44);
    Boton botonActualizar("Actualizar ahora", 20, 300, 180, 44);
    Boton botonSalir("Salir", 20, 535, 180, 44);

    // ---------------- Estado del programa ----------------
    Pantalla pantalla = Pantalla::LOGIN;
    Vista vista = Vista::USUARIOS;
    int scroll = 0;              // primera fila visible de la tabla
    int scrollMaximo = 0;        // hasta donde se puede bajar
    string mensajePanel;         // errores de red mostrados abajo
    sf::Clock relojRefresco;     // para actualizar solo cada X segundos

    // Pide al servidor los datos de la vista actual.
    auto cargarVista = [&]() {
        bool ok = false;

        if (vista == Vista::USUARIOS)         ok = admin.cargarUsuarios();
        else if (vista == Vista::CAFETERIAS)  ok = admin.cargarCafeterias();
        else                                  ok = admin.cargarUsuariosEnLinea();

        mensajePanel = ok ? "" : ("Sin datos: " + admin.getUltimoError());
        relojRefresco.restart();
    };

    auto cambiarVista = [&](Vista nueva) {
        vista = nueva;
        scroll = 0;
        cargarVista();
    };

    auto intentarLogin = [&]() {
        if (admin.iniciarSesionAdmin(campoUsuario.getContenido(), campoClave.getContenido())) {
            pantalla = Pantalla::PANEL;
            mensajeLogin.clear();
            cambiarVista(Vista::USUARIOS);
        } else {
            mensajeLogin = admin.getUltimoError();
            campoClave.limpiar();
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

                    botonUsuarios.actualizarHover(mx, my);
                    botonCafeterias.actualizarHover(mx, my);
                    botonEnLinea.actualizarHover(mx, my);
                    botonActualizar.actualizarHover(mx, my);
                    botonSalir.actualizarHover(mx, my);
                }
                else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    float mx = static_cast<float>(e.mouseButton.x);
                    float my = static_cast<float>(e.mouseButton.y);

                    if (botonUsuarios.contiene(mx, my))        cambiarVista(Vista::USUARIOS);
                    else if (botonCafeterias.contiene(mx, my)) cambiarVista(Vista::CAFETERIAS);
                    else if (botonEnLinea.contiene(mx, my))    cambiarVista(Vista::EN_LINEA);
                    else if (botonActualizar.contiene(mx, my)) cargarVista();
                    else if (botonSalir.contiene(mx, my))      ventana.cerrar();
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
        // Asi se ve en "casi tiempo real" quien se registra o se conecta,
        // sin hilos ni nada complicado: es el mismo ciclo, con un reloj.
        if (pantalla == Pantalla::PANEL &&
            relojRefresco.getElapsedTime().asSeconds() >= SEGUNDOS_REFRESCO) {
            cargarVista();
        }

        // ---------- 3) DIBUJAR ----------
        win.clear(FONDO);

        if (pantalla == Pantalla::LOGIN) {

            ventana.dibujarTextoCentrado("UPIIFOOD", sf::FloatRect(0, 70, ANCHO, 60), 52, BARRA_LATERAL);
            ventana.dibujarTextoCentrado("Acceso de administrador", sf::FloatRect(0, 135, ANCHO, 30), 22, TEXTO_SUAVE);

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
            ventana.dibujarTexto("Administrador", 20, 62, 16, sf::Color(200, 180, 160));

            botonUsuarios.setActivo(vista == Vista::USUARIOS);
            botonCafeterias.setActivo(vista == Vista::CAFETERIAS);
            botonEnLinea.setActivo(vista == Vista::EN_LINEA);

            botonUsuarios.dibujar(ventana);
            botonCafeterias.dibujar(ventana);
            botonEnLinea.dibujar(ventana);
            botonActualizar.dibujar(ventana);
            botonSalir.dibujar(ventana);

            ventana.dibujarTexto("Sesión de:", 20, 470, 13, sf::Color(200, 180, 160));
            ventana.dibujarTexto(recortar(admin.getNombre(), 18), 20, 490, 16, CREMA);

            // --- zona de contenido ---
            rectangulo(ventana, 240, 15, 650, 570, sf::Color(255, 255, 255));

            if (vista == Vista::USUARIOS) {
                const vector<Usuario>& lista = admin.getListaUsuarios();
                int total = static_cast<int>(lista.size());
                scrollMaximo = max(0, total - FILAS_VISIBLES);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Usuarios registrados (" + to_string(total) + ")",
                                "En el orden en que se registraron. Rueda del mouse para bajar.",
                                {{"#", 260}, {"Nombre", 300}, {"Username", 500}, {"Tipo", 700}});

                for (int i = 0; i < FILAS_VISIBLES && scroll + i < total; i++) {
                    const Usuario& u = lista[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i);
                    ventana.dibujarTexto(to_string(scroll + i + 1), 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(u.getNombre(), 20), 300, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(recortar(u.getUsername(), 16), 500, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(u.getTipoUsuario(), 700, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Todavía no hay usuarios registrados.", 260, 150, 18, TEXTO_SUAVE);
                }
            }
            else if (vista == Vista::CAFETERIAS) {
                const vector<ResumenCafeteria>& lista = admin.getResumenCafeterias();
                int total = static_cast<int>(lista.size());
                scrollMaximo = max(0, total - FILAS_VISIBLES);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "Cafeterías (" + to_string(total) + ")",
                                "Con la cantidad de pedidos que tiene cada una.",
                                {{"ID", 260}, {"Cafetería", 320}, {"Usuario", 560}, {"Pedidos", 770}});

                for (int i = 0; i < FILAS_VISIBLES && scroll + i < total; i++) {
                    const ResumenCafeteria& c = lista[scroll + i];
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i);
                    ventana.dibujarTexto(c.idCafeteria, 260, y, 18, TEXTO_SUAVE);
                    ventana.dibujarTexto(recortar(c.nombre, 22), 320, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(recortar(c.username, 14), 560, y, 18, TEXTO_OSCURO);
                    ventana.dibujarTexto(to_string(c.pedidos), 790, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Todavía no hay cafeterías registradas.", 260, 150, 18, TEXTO_SUAVE);
                }
            }
            else { // EN_LINEA
                const vector<string>& lista = admin.getUsuariosEnLinea();
                int total = static_cast<int>(lista.size());
                scrollMaximo = max(0, total - FILAS_VISIBLES);
                scroll = min(scroll, scrollMaximo);

                encabezadoTabla(ventana, "En línea ahora (" + to_string(total) + ")",
                                "Se actualiza solo cada 3 segundos.",
                                {{"Usuario", 300}});

                for (int i = 0; i < FILAS_VISIBLES && scroll + i < total; i++) {
                    float y = TABLA_Y_FILAS + i * ALTO_FILA;

                    franja(ventana, i);

                    // puntito verde = conectado
                    sf::CircleShape punto(6);
                    punto.setFillColor(VERDE);
                    punto.setPosition(268, y + 6);
                    win.draw(punto);

                    ventana.dibujarTexto(recortar(lista[scroll + i], 30), 300, y, 18, TEXTO_OSCURO);
                }

                if (total == 0 && mensajePanel.empty()) {
                    ventana.dibujarTexto("Nadie conectado en este momento.", 260, 150, 18, TEXTO_SUAVE);
                }
            }

            // --- mensaje de error de red (si lo hay) ---
            if (!mensajePanel.empty()) {
                ventana.dibujarTexto(mensajePanel, 260, 562, 15, ROJO_ERROR);
            }
        }

        win.display();
    }

    // Al salir del ciclo, el destructor de "admin" cierra el socket y el
    // servidor nos marca automaticamente como desconectados.
    return 0;
}
