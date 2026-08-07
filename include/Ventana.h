#ifndef VENTANA_H
#define VENTANA_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "BaseDatos.h"

class Ventana {
private:
    sf::RenderWindow window;
    sf::Font font;
    std::vector<sf::Text> textos;

public:
    Ventana(const std::string& nombreVentana, int ancho, int alto);
    void agregarTexto(const std::string& texto, int x, int y, int size=20);
    void loop();
    void limpiar();
};

#endif
