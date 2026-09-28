#ifndef VENTANA_H
#define VENTANA_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <mutex>

class Ventana {
private:
    sf::RenderWindow window;
    sf::Font font;
    std::vector<sf::Text> textos;

    // Lineas "en vivo" (por ejemplo, los ultimos eventos del servidor). Las
    // escribe OTRO hilo (el monitor) y las lee el hilo que dibuja, por eso van
    // protegidas con un mutex.
    std::vector<std::string> lineasVivas;
    mutable std::mutex mutexLineas;

public:
    Ventana(const std::string& nombreVentana, int ancho, int alto);

    // Agrega una linea de texto a la lista de cosas que se dibujan cada frame.
    void agregarTexto(const std::string& texto, int x, int y, int size=20);

    // Reemplaza las lineas en vivo (seguro para llamarse desde otro hilo).
    void setLineasVivas(const std::vector<std::string>& lineas);

    // Corre el ciclo de eventos/dibujado. Bloquea hasta que se cierra la ventana.
    void loop();

    // Quita todos los textos agregados (para "refrescar" la pantalla).
    void limpiar();

    bool estaAbierta() const;
};

#endif
