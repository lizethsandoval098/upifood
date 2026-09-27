#include "Ventana.h"

#include <iostream>

using namespace std;


Ventana::Ventana(
    const string& nombreVentana,
    int ancho,
    int alto
)
    : window(
        sf::VideoMode(ancho, alto),
        nombreVentana
      )
{
    if(!font.loadFromFile("recursos/font.ttf")) {

        cout
            << "No se pudo cargar recursos/font.ttf"
            << endl;
    }

    window.setFramerateLimit(60);
}


bool Ventana::estaAbierta() const {

    return window.isOpen();
}


void Ventana::cerrar() {

    window.close();
}


void Ventana::limpiar() {

    window.clear(
        sf::Color(245, 245, 245)
    );
}


void Ventana::mostrar() {

    window.display();
}


bool Ventana::obtenerEvento(
    sf::Event& evento
) {

    return window.pollEvent(evento);
}


bool Ventana::botonPresionado(
    float x,
    float y,
    float ancho,
    float alto
) {

    sf::Vector2i posicion =
        sf::Mouse::getPosition(window);

    if(
        posicion.x >= x &&
        posicion.x <= x + ancho &&
        posicion.y >= y &&
        posicion.y <= y + alto
    ) {

        return true;
    }

    return false;
}


void Ventana::dibujarTexto(
    const string& texto,
    float x,
    float y,
    unsigned int tamano
) {

    sf::Text textoSFML;

    textoSFML.setFont(font);
    textoSFML.setString(texto);
    textoSFML.setCharacterSize(tamano);
    textoSFML.setFillColor(
        sf::Color(30, 30, 30)
    );

    textoSFML.setPosition(x, y);

    window.draw(textoSFML);
}


void Ventana::dibujarBoton(
    const string& texto,
    float x,
    float y,
    float ancho,
    float alto
) {

    sf::RectangleShape boton;

    boton.setSize(
        sf::Vector2f(ancho, alto)
    );

    boton.setPosition(x, y);

    boton.setFillColor(
        sf::Color(70, 130, 180)
    );

    boton.setOutlineThickness(2);

    boton.setOutlineColor(
        sf::Color(30, 80, 120)
    );

    window.draw(boton);


    sf::Text textoBoton;

    textoBoton.setFont(font);
    textoBoton.setString(texto);
    textoBoton.setCharacterSize(20);

    textoBoton.setFillColor(
        sf::Color::White
    );


    sf::FloatRect limites =
        textoBoton.getLocalBounds();


    textoBoton.setPosition(
        x + (ancho - limites.width) / 2,
        y + (alto - limites.height) / 2 - 5
    );

    window.draw(textoBoton);
}
