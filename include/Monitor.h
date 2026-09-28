#ifndef MONITOR_H
#define MONITOR_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>

// Monitor en vivo: un HILO con su PROPIO socket que le pregunta al servidor
// cada segundo "que eventos han pasado desde el ultimo que vi" (EVENTOS|id).
// Usa un socket aparte para no mezclar sus respuestas con las del menu
// principal, que usa el socket original.
//
// Lo usan las pantallas de Administrador y Cafeteria para ver pedidos, pagos,
// cambios de estado, restock, logins... aunque no haya nadie "interactuando":
// los clientes simulados (simulador) generan la actividad.
class MonitorEventos {
private:
    std::string ip;
    int puerto;
    std::string filtroCafeteria;   // "" = todos los eventos, "1"/"2" = solo esa cafeteria

    std::thread hilo;
    std::atomic<bool> corriendo;
    std::function<void(const std::string&)> alEvento;

    void ciclo();

public:
    MonitorEventos(const std::string& ip, int puerto, const std::string& filtroCafeteria = "");
    ~MonitorEventos();

    // Arranca el hilo. 'alEvento' se llama (desde el hilo del monitor) con una
    // linea ya formateada por cada evento nuevo.
    bool iniciar(std::function<void(const std::string&)> alEvento);
    void detener();
};

// Version de consola: imprime los eventos en vivo hasta que el usuario
// presione ENTER (o Ctrl+C) y regresa al menu.
void monitorEnVivoConsola(const std::string& titulo, const std::string& filtroCafeteria = "");

#endif
