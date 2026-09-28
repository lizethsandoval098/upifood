#ifndef VISTAENVIVO_H
#define VISTAENVIVO_H

#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "Ventana.h"
#include "Monitor.h"
#include "Protocolo.h"

// Junta MonitorEventos (hilo que consulta al servidor) con Ventana (SFML):
// abre la ventana y la mantiene actualizada con los ultimos eventos mientras
// este abierta. Se usa desde las pantallas de Administrador y Cafeteria.
inline void abrirVentanaEnVivo(const std::string& titulo, const std::string& encabezado,
                               const std::string& filtroCafeteria = "") {
    Ventana ventana(titulo, 900, 620);
    ventana.agregarTexto(encabezado, 20, 20, 30);
    ventana.agregarTexto("Eventos en vivo (se actualizan solos cada segundo):", 20, 90, 18);

    std::deque<std::string> ultimos;
    std::mutex mtx;

    MonitorEventos monitor(obtenerIpServidor(), obtenerPuertoServidor(), filtroCafeteria);

    monitor.iniciar([&](const std::string& linea) {
        std::vector<std::string> copia;
        {
            std::lock_guard<std::mutex> guard(mtx);
            ultimos.push_back(linea);
            while (ultimos.size() > 18) ultimos.pop_front();
            copia.assign(ultimos.begin(), ultimos.end());
        }
        ventana.setLineasVivas(copia);
    });

    ventana.loop();     // bloquea (hilo principal) hasta cerrar la ventana
    monitor.detener();
}

#endif
