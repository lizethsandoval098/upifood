#include "Simulador.h"
#include "Protocolo.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <random>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

namespace {

// ---------------------------------------------------------------------
// Estado compartido
// ---------------------------------------------------------------------

// --- proceso PADRE ---
pid_t g_hijos[2] = {-1, -1};

void padreReenviarTerminacion(int) {
    // kill() es async-signal-safe: se puede llamar desde un handler.
    for (pid_t h : g_hijos) if (h > 0) kill(h, SIGTERM);
}

void padreReenviarUsr1(int) {
    for (pid_t h : g_hijos) if (h > 0) kill(h, SIGUSR1);
}

// --- proceso HIJO ---
// std::atomic<int> lock-free es seguro de escribir desde un handler y de
// leer desde varios hilos sin condicion de carrera.
atomic<int> g_detener(0);
atomic<int> g_imprimirAvance(0);

void hijoManejadorTerminacion(int) { g_detener.store(1); }
void hijoManejadorUsr1(int) { g_imprimirAvance.store(1); }

struct ProductoSim {
    string id;
    string nombre;
    int stock;
    float precio;
};

struct Contadores {
    atomic<int> creados{0};
    atomic<int> aprobados{0};
    atomic<int> rechazados{0};
    atomic<int> sinStock{0};
    atomic<int> entregados{0};
    atomic<int> errores{0};
    atomic<long> centavosAprobados{0};
};

mutex g_mtxConsola;

void imprimir(const string& linea) {
    lock_guard<mutex> guard(g_mtxConsola);
    cout << linea << endl;
}

void dormir(int ms) {
    // en rebanadas de 50 ms para reaccionar rapido a SIGTERM
    while (ms > 0 && !g_detener.load()) {
        int paso = min(ms, 50);
        this_thread::sleep_for(chrono::milliseconds(paso));
        ms -= paso;
    }
}

// Genera un numero de tarjeta de 16 digitos que cumple (o no) el algoritmo de Luhn.
string generarTarjeta(mt19937& rng, bool valida) {
    string n = "4";
    uniform_int_distribution<int> digito(0, 9);
    for (int i = 0; i < 14; i++) n += static_cast<char>('0' + digito(rng));

    int suma = 0;
    bool duplicar = true;   // el digito verificador (que falta) ocupa la posicion 1
    for (int i = static_cast<int>(n.size()) - 1; i >= 0; i--) {
        int d = n[i] - '0';
        if (duplicar) { d *= 2; if (d > 9) d -= 9; }
        suma += d;
        duplicar = !duplicar;
    }

    int verificador = (10 - suma % 10) % 10;
    if (!valida) verificador = (verificador + 1 + static_cast<int>(rng() % 8)) % 10;  // distinto del correcto
    n += static_cast<char>('0' + verificador);
    return n;
}

vector<ProductoSim> pedirCatalogo(int socket, int cafe) {
    vector<ProductoSim> catalogo;

    vector<string> enc = separarCampos(enviarComando(socket, "INVENTARIO|" + to_string(cafe)));
    if (enc.size() < 2 || enc[0] != "OK") return catalogo;

    int n = 0;
    try { n = stoi(enc[1]); } catch (...) { return catalogo; }

    for (const string& linea : leerLineas(socket, n)) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 4) continue;

        try {
            catalogo.push_back({c[0], c[1], stoi(c[2]), stof(c[3])});
        } catch (...) {}
    }

    return catalogo;
}

// ---------------------------------------------------------------------
// HILO CLIENTE: un cliente virtual con su propio socket
// ---------------------------------------------------------------------

struct ContextoCafeteria {
    int cafe;
    const OpcionesSimulador* op;
    Contadores* cont;
    vector<ProductoSim> catalogo;

    mutex mtxVendidas;                 // protege 'vendidas'
    map<string, int> vendidas;         // unidades netas vendidas por producto (solo pagos aprobados)

    atomic<int> clientesActivos{0};
};

void hiloCliente(ContextoCafeteria* ctx, int indice) {
    const OpcionesSimulador& op = *ctx->op;
    Contadores& cont = *ctx->cont;

    char etiqueta[48];
    snprintf(etiqueta, sizeof(etiqueta), "[C%d cliente%02d]", ctx->cafe, indice);

    int sock = conectarAlServidor(op.ip, op.puerto);

    if (sock == -1) {
        imprimir(string(etiqueta) + " no pudo conectarse al servidor");
        cont.errores++;
        ctx->clientesActivos--;
        return;
    }

    mt19937 rng(op.semilla + static_cast<unsigned>(ctx->cafe * 1000 + indice));

    char usuario[32];
    snprintf(usuario, sizeof(usuario), "sim_c%d_%02d", ctx->cafe, indice);
    string username = usuario;

    // REGISTRO_CLIENTE|username|nombre|correo|contrasena|apP|apM|tipo  (si ya existe, no pasa nada)
    enviarComando(sock, "REGISTRO_CLIENTE|" + username + "|Sim" + to_string(indice) + "|" + username +
                        "@alumno.ipn.mx|simulador1|Virtual|Hilo|IPN");

    vector<string> login = separarCampos(enviarComando(sock, "LOGIN_CLIENTE|" + username + "|simulador1"));

    if (login.empty() || login[0] != "OK") {
        imprimir(string(etiqueta) + " no pudo iniciar sesion");
        cont.errores++;
        close(sock);
        ctx->clientesActivos--;
        return;
    }

    imprimir(string(etiqueta) + " en linea como " + username);

    uniform_int_distribution<int> cantidadDist(1, 3);
    const int pausaMin = max(0, op.ritmoMs / 2);
    const int pausaMax = max(pausaMin, op.ritmoMs * 3 / 2);
    uniform_int_distribution<int> pausaDist(pausaMin, pausaMax);

    for (int k = 0; k < op.pedidosPorCliente && !g_detener.load(); k++) {
        dormir(pausaDist(rng));
        if (g_detener.load()) break;

        // ---- armar carrito ----
        map<string, int> carrito;

        if (op.carrera) {
            // todos contra el mismo producto: maxima contencion
            carrito[ctx->catalogo[0].id] = 2 + cantidadDist(rng);
        } else {
            int lineas = 1 + static_cast<int>(rng() % 3);
            for (int i = 0; i < lineas; i++) {
                const ProductoSim& p = ctx->catalogo[rng() % ctx->catalogo.size()];
                carrito[p.id] += cantidadDist(rng);
            }
        }

        string items;
        for (const auto& par : carrito) {
            items += (items.empty() ? "" : ";") + par.first + ":" + to_string(par.second);
        }

        // ---- CREAR_PEDIDO ----
        vector<string> r = separarCampos(enviarComando(sock, "CREAR_PEDIDO|" + username + "|" +
                                                             to_string(ctx->cafe) + "|" + items));

        if (r.empty() || r[0] != "OK" || r.size() < 3) {
            string motivo = r.size() > 1 ? r[1] : "sin respuesta";

            if (motivo.find("Sin stock") != string::npos) {
                cont.sinStock++;
                imprimir(string(etiqueta) + " pedido rechazado: " + motivo);
            } else {
                cont.errores++;
                imprimir(string(etiqueta) + " ERROR al crear pedido: " + motivo);
            }
            continue;
        }

        string folio = r[1];
        cont.creados++;
        imprimir(string(etiqueta) + " creo pedido " + folio + " (" + items + ") $" + r[2]);

        // ---- PAGAR (aprox. 1 de cada 7 pagos usa una tarjeta invalida) ----
        bool tarjetaBuena = (rng() % 7) != 0;
        vector<string> p = separarCampos(enviarComando(sock, "PAGAR|" + folio + "|" + generarTarjeta(rng, tarjetaBuena)));

        if (!p.empty() && p[0] == "OK") {
            cont.aprobados++;
            try { cont.centavosAprobados += static_cast<long>(stod(r[2]) * 100.0 + 0.5); } catch (...) {}

            {
                lock_guard<mutex> guard(ctx->mtxVendidas);
                for (const auto& par : carrito) ctx->vendidas[par.first] += par.second;
            }

            imprimir(string(etiqueta) + " pago APROBADO de " + folio);
        } else {
            cont.rechazados++;
            imprimir(string(etiqueta) + " pago RECHAZADO de " + folio + " (pedido cancelado)");
            continue;
        }

        // ---- esperar a que la cafeteria lo deje "Listo" y recogerlo ----
        if (op.esperaEntregaMs > 0) {
            int esperado = 0;

            while (esperado < op.esperaEntregaMs && !g_detener.load()) {
                vector<string> e = separarCampos(enviarComando(sock, "ESTADO_PEDIDO|" + folio));

                if (e.size() >= 3 && e[0] == "OK" && e[2] == "Listo") {
                    vector<string> ent = separarCampos(enviarComando(sock, "CAMBIAR_ESTADO|" + folio + "|Entregado"));

                    if (!ent.empty() && ent[0] == "OK") {
                        cont.entregados++;
                        imprimir(string(etiqueta) + " recogio su pedido " + folio);
                    }
                    break;
                }

                dormir(250);
                esperado += 250;
            }
        }
    }

    close(sock);
    ctx->clientesActivos--;
}

// ---------------------------------------------------------------------
// HILO CAFETERIA (opcional): prepara los pedidos pagados
// ---------------------------------------------------------------------

void hiloCafeteriaAuto(ContextoCafeteria* ctx) {
    const OpcionesSimulador& op = *ctx->op;

    int sock = conectarAlServidor(op.ip, op.puerto);
    if (sock == -1) return;

    char etiqueta[48];
    snprintf(etiqueta, sizeof(etiqueta), "[C%d cafeteria-auto]", ctx->cafe);

    int vueltasSinClientes = 0;

    while (!g_detener.load()) {
        vector<string> enc = separarCampos(enviarComando(sock, "LISTAR_PEDIDOS|" + to_string(ctx->cafe)));

        if (enc.size() >= 2 && enc[0] == "OK") {
            int n = 0;
            try { n = stoi(enc[1]); } catch (...) {}

            for (const string& linea : leerLineas(sock, n)) {
                vector<string> c = separarCampos(linea);
                if (c.size() < 6) continue;

                // folio|fecha|estado|total|user|pagado
                if (c[2] == "Pendiente" && c[5] == "1") {
                    vector<string> a = separarCampos(enviarComando(sock, "CAMBIAR_ESTADO|" + c[0] + "|Preparando"));

                    if (!a.empty() && a[0] == "OK") {
                        imprimir(string(etiqueta) + " preparando " + c[0]);
                        dormir(300 + static_cast<int>(rand() % 400));
                        enviarComando(sock, "CAMBIAR_ESTADO|" + c[0] + "|Listo");
                        imprimir(string(etiqueta) + " pedido " + c[0] + " LISTO");
                    }
                }
            }
        }

        if (ctx->clientesActivos.load() == 0) {
            if (++vueltasSinClientes >= 2) break;   // una pasada final y termina
        }

        dormir(300);
    }

    close(sock);
}

// ---------------------------------------------------------------------
// Trabajo completo de UN proceso hijo (una cafeteria)
// ---------------------------------------------------------------------

int trabajoCafeteria(int cafe, const OpcionesSimulador& op, int fdResumen) {
    // Handlers propios del hijo (el padre reenvia SIGTERM / SIGUSR1).
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = hijoManejadorTerminacion;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    sa.sa_handler = hijoManejadorUsr1;
    sigaction(SIGUSR1, &sa, nullptr);

    ContextoCafeteria ctx;
    ctx.cafe = cafe;
    ctx.op = &op;

    Contadores cont;
    ctx.cont = &cont;

    // Catalogo y stock inicial (antes de lanzar hilos)
    int sockInicial = conectarAlServidor(op.ip, op.puerto);
    if (sockInicial == -1) {
        imprimir("[C" + to_string(cafe) + "] no se pudo conectar al servidor " + op.ip + ":" + to_string(op.puerto));
        string linea = "RESUMEN|" + to_string(cafe) + "|0|0|0|0|0|1|0|-1\n";
        ssize_t w = write(fdResumen, linea.c_str(), linea.size());
        (void)w;
        return 1;
    }

    ctx.catalogo = pedirCatalogo(sockInicial, cafe);
    close(sockInicial);

    if (ctx.catalogo.empty()) {
        imprimir("[C" + to_string(cafe) + "] la cafeteria no tiene productos (¿inventario vacio?)");
        string linea = "RESUMEN|" + to_string(cafe) + "|0|0|0|0|0|1|0|-1\n";
        ssize_t w = write(fdResumen, linea.c_str(), linea.size());
        (void)w;
        return 1;
    }

    map<string, int> stockInicial;
    for (const auto& p : ctx.catalogo) stockInicial[p.id] = p.stock;

    imprimir("[C" + to_string(cafe) + "] proceso hijo pid " + to_string(getpid()) + " lanzando " +
             to_string(op.clientesPorCafeteria) + " hilos-cliente" + (op.autoCafeteria ? " + hilo cafeteria" : ""));

    // ---- lanzar hilos ----
    vector<thread> hilos;
    ctx.clientesActivos = op.clientesPorCafeteria;

    for (int i = 1; i <= op.clientesPorCafeteria; i++) {
        hilos.emplace_back(hiloCliente, &ctx, i);
    }

    thread hiloCafe;
    if (op.autoCafeteria) hiloCafe = thread(hiloCafeteriaAuto, &ctx);

    // El hilo principal del hijo vigila el avance y atiende SIGUSR1.
    while (ctx.clientesActivos.load() > 0) {
        this_thread::sleep_for(chrono::milliseconds(200));

        if (g_imprimirAvance.exchange(0) == 1) {
            imprimir("[C" + to_string(cafe) + "] AVANCE (SIGUSR1): pedidos=" + to_string(cont.creados.load()) +
                     " aprobados=" + to_string(cont.aprobados.load()) +
                     " rechazados=" + to_string(cont.rechazados.load()) +
                     " sinStock=" + to_string(cont.sinStock.load()));
        }
    }

    for (auto& h : hilos) h.join();
    if (hiloCafe.joinable()) hiloCafe.join();

    // ---- verificacion de consistencia (¿hubo condiciones de carrera?) ----
    int consistente = -1;
    string detalle;

    int sockFinal = conectarAlServidor(op.ip, op.puerto);
    if (sockFinal != -1) {
        vector<ProductoSim> finales = pedirCatalogo(sockFinal, cafe);
        close(sockFinal);

        consistente = 1;

        for (const auto& p : finales) {
            auto it = stockInicial.find(p.id);
            if (it == stockInicial.end()) continue;

            int vendidas = 0;
            {
                lock_guard<mutex> guard(ctx.mtxVendidas);
                auto v = ctx.vendidas.find(p.id);
                if (v != ctx.vendidas.end()) vendidas = v->second;
            }

            int esperado = it->second - vendidas;

            if (p.stock < 0 || p.stock != esperado) {
                consistente = 0;
                detalle += p.id + "(esperado " + to_string(esperado) + " real " + to_string(p.stock) + ") ";
            }
        }
    }

    if (consistente == 0) {
        imprimir("[C" + to_string(cafe) + "] INCONSISTENCIA de inventario: " + detalle +
                 "(si alguien mas movio el inventario durante la corrida, es normal)");
    } else if (consistente == 1) {
        imprimir("[C" + to_string(cafe) + "] inventario consistente: stock final = stock inicial - unidades vendidas");
    }

    char resumen[256];
    snprintf(resumen, sizeof(resumen), "RESUMEN|%d|%d|%d|%d|%d|%d|%d|%ld|%d\n", cafe,
             cont.creados.load(), cont.aprobados.load(), cont.rechazados.load(), cont.sinStock.load(),
             cont.entregados.load(), cont.errores.load(), cont.centavosAprobados.load(), consistente);

    ssize_t w = write(fdResumen, resumen, strlen(resumen));   // < PIPE_BUF: escritura atomica
    (void)w;

    return 0;
}

}  // namespace

// =====================================================================
//  PROCESO PADRE
// =====================================================================

int ejecutarSimulacion(const OpcionesSimulador& opcionesEntrada) {
    OpcionesSimulador op = opcionesEntrada;

    if (op.semilla == 0) op.semilla = static_cast<unsigned>(time(nullptr));

    signal(SIGPIPE, SIG_IGN);

    cout << "==== SIMULADOR UPIIFOOD ====" << endl;
    cout << "servidor " << op.ip << ":" << op.puerto << " | " << op.clientesPorCafeteria
         << " clientes(hilos) x cafeteria | " << op.pedidosPorCliente << " pedidos c/u"
         << (op.carrera ? " | MODO CARRERA" : "")
         << (op.autoCafeteria ? " | cafeteria automatica" : "") << endl;
    cout << "Tip: abre el Administrador o la Cafeteria y elige MONITOR EN VIVO para ver todo." << endl;
    cout << "Senales al pid " << getpid() << ": kill -USR1 (avance) | Ctrl+C / kill -TERM (detener)\n" << endl;

    // Prueba de conexion antes de crear procesos
    int prueba = conectarAlServidor(op.ip, op.puerto, 5);
    if (prueba == -1) {
        cout << "No se pudo conectar al servidor. ¿Esta corriendo 'servidor'? (UPIIFOOD_IP / --ip)" << endl;
        return 1;
    }
    close(prueba);

    // ---- pipe hijos -> padre ----
    int pipeResumen[2];
    if (pipe(pipeResumen) == -1) {
        perror("pipe");
        return 1;
    }

    // Handlers del padre: se instalan ANTES del fork (los hijos los reemplazan).
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // sin SA_RESTART: el read() del padre se interrumpe y revisa

    sa.sa_handler = padreReenviarTerminacion;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    sa.sa_handler = padreReenviarUsr1;
    sigaction(SIGUSR1, &sa, nullptr);

    vector<int> cafeterias;
    if (op.soloUnaCafeteria) cafeterias.push_back(op.cafeteriaUnica);
    else { cafeterias.push_back(1); cafeterias.push_back(2); }

    // ---- fork(): un hijo por cafeteria ----
    int lanzados = 0;

    for (size_t i = 0; i < cafeterias.size(); i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // ---------- HIJO ----------
            close(pipeResumen[0]);
            int codigo = trabajoCafeteria(cafeterias[i], op, pipeResumen[1]);
            close(pipeResumen[1]);
            fflush(stdout);
            _exit(codigo);
        }

        g_hijos[i] = pid;
        lanzados++;
    }

    close(pipeResumen[1]);   // el padre solo lee (si no lo cierra, nunca ve EOF)

    if (lanzados == 0) {
        close(pipeResumen[0]);
        return 1;
    }

    // ---- leer resumenes hasta EOF (todos los hijos cerraron su extremo) ----
    string acumulado;
    vector<string> resumenes;
    char buffer[512];

    while (true) {
        ssize_t n = read(pipeResumen[0], buffer, sizeof(buffer));

        if (n > 0) {
            acumulado.append(buffer, static_cast<size_t>(n));
            size_t pos;
            while ((pos = acumulado.find('\n')) != string::npos) {
                resumenes.push_back(acumulado.substr(0, pos));
                acumulado.erase(0, pos + 1);
            }
        } else if (n == 0) {
            break;
        } else if (errno != EINTR) {
            break;
        }
    }

    close(pipeResumen[0]);

    // ---- recoger a los hijos (evita zombies) ----
    bool algunoFallo = false;

    for (int i = 0; i < 2; i++) {
        if (g_hijos[i] <= 0) continue;

        int estado = 0;
        while (waitpid(g_hijos[i], &estado, 0) == -1 && errno == EINTR) {}

        if (WIFSIGNALED(estado)) {
            cout << "El hijo " << g_hijos[i] << " murio por la senal " << WTERMSIG(estado) << endl;
            algunoFallo = true;
        } else if (WIFEXITED(estado) && WEXITSTATUS(estado) != 0) {
            algunoFallo = true;
        }

        g_hijos[i] = -1;
    }

    // ---- tabla final ----
    cout << "\n================= RESUMEN DE LA SIMULACION =================" << endl;
    cout << left << setw(11) << "Cafeteria" << setw(9) << "Pedidos" << setw(10) << "Pagos OK"
         << setw(12) << "Rechazados" << setw(10) << "SinStock" << setw(12) << "Entregados"
         << setw(9) << "Errores" << setw(12) << "Monto" << "Inventario" << endl;

    double montoTotal = 0;
    int pedidosTotal = 0;

    for (const string& linea : resumenes) {
        vector<string> c = separarCampos(linea);
        if (c.size() < 10 || c[0] != "RESUMEN") continue;

        double monto = atol(c[8].c_str()) / 100.0;
        montoTotal += monto;
        pedidosTotal += atoi(c[2].c_str());

        int cons = atoi(c[9].c_str());

        cout << left << setw(11) << c[1] << setw(9) << c[2] << setw(10) << c[3] << setw(12) << c[4]
             << setw(10) << c[5] << setw(12) << c[6] << setw(9) << c[7]
             << "$" << setw(11) << fixed << setprecision(2) << monto
             << (cons == 1 ? "consistente" : cons == 0 ? "INCONSISTENTE" : "no verificado") << endl;
    }

    cout << "Total: " << pedidosTotal << " pedidos, $" << fixed << setprecision(2) << montoTotal
         << " en pagos aprobados." << endl;

    // ---- estadisticas de las matrices del servidor ----
    int sock = conectarAlServidor(op.ip, op.puerto, 5);
    if (sock != -1) {
        vector<string> enc = separarCampos(enviarComando(sock, "ESTADISTICAS"));

        if (enc.size() >= 2 && enc[0] == "OK") {
            int n = 0;
            try { n = stoi(enc[1]); } catch (...) {}

            cout << "\nMatriz pedidosPorEstado del servidor:" << endl;
            cout << left << setw(11) << "Cafeteria" << setw(8) << "Stock" << setw(10) << "Vendidas"
                 << setw(11) << "Pendiente" << setw(12) << "Preparando" << setw(7) << "Listo"
                 << setw(11) << "Entregado" << "Cancelado" << endl;

            for (const string& linea : leerLineas(sock, n)) {
                vector<string> c = separarCampos(linea);
                if (c.size() < 8) continue;

                cout << left << setw(11) << c[0] << setw(8) << c[1] << setw(10) << c[2]
                     << setw(11) << c[3] << setw(12) << c[4] << setw(7) << c[5]
                     << setw(11) << c[6] << c[7] << endl;
            }
        }

        close(sock);
    }

    return algunoFallo ? 1 : 0;
}
