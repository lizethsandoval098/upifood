#ifndef VENTANA_H
#define VENTANA_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

using namespace std;

class Ventana {

private:

    sf::RenderWindow window;
    sf::Font font;

public:

    Ventana(
        const string& nombreVentana,
        int ancho,
        int alto
    );

    bool estaAbierta() const;

    void cerrar();

    bool botonPresionado(
        float x,
        float y,
        float ancho,
        float alto
    );

    void dibujarTexto(
        const string& texto,
        float x,
        float y,
        unsigned int tamano
    );

    void dibujarBoton(
        const string& texto,
        float x,
        float y,
        float ancho,
        float alto
    );

    void limpiar();

    void mostrar();

    bool obtenerEvento(
        sf::Event& evento
    );
};

#endif
