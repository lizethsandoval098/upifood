#ifndef VENTANA_H
#define VENTANA_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class Ventana {
private:
    sf::RenderWindow window;
    sf::Font font;
    std::vector<sf::Text> textos;

public:
    Ventana(const std::string& nombreVentana, int ancho, int alto);

    // Agrega una linea de texto a la lista de cosas que se dibujan cada frame.
    void agregarTexto(const std::string& texto, int x, int y, int size=20);

    // Corre el ciclo de eventos/dibujado. Bloquea hasta que se cierra la ventana.
    void loop();

    // Quita todos los textos agregados (para "refrescar" la pantalla).
    void limpiar();

    bool estaAbierta() const;
};

#endif
