#include "Ventana.h"
#include <iostream>

// NOTA IMPORTANTE: esto esta escrito para SFML 2.5/2.6 (la version que se
// suele usar en clase). Si en tu maquina tienen instalada SFML 3.x, la forma
// de crear sf::Text y de leer eventos cambio (sf::Text ahora pide la fuente
// en el constructor, y pollEvent regresa std::optional). Si al compilar te
// marca error justo en esas dos partes, es por eso.

Ventana::Ventana(const std::string& nombreVentana, int ancho, int alto)
    : window(sf::VideoMode(ancho, alto), nombreVentana) {

    // Necesitan poner un archivo .ttf real en recursos/ y ajustar esta ruta.
    if (!font.loadFromFile("recursos/font.ttf")) {
        std::cout << "[Ventana] Aviso: no se pudo cargar recursos/font.ttf. "
                  << "El texto no se va a ver hasta que agreguen una fuente real ahi."
                  << std::endl;
    }
}

void Ventana::agregarTexto(const std::string& texto, int x, int y, int size) {
    sf::Text t;
    t.setFont(font);
    t.setString(texto);
    t.setCharacterSize(size);
    t.setPosition(static_cast<float>(x), static_cast<float>(y));

    textos.push_back(t);
}

void Ventana::limpiar() {
    textos.clear();
}

void Ventana::setLineasVivas(const std::vector<std::string>& lineas) {
    std::lock_guard<std::mutex> guard(mutexLineas);
    lineasVivas = lineas;
}

bool Ventana::estaAbierta() const {
    return window.isOpen();
}

void Ventana::loop() {
    while (window.isOpen()) {
        sf::Event evento;

        while (window.pollEvent(evento)) {
            if (evento.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color(245, 245, 245));

        for (const auto& t : textos) {
            window.draw(t);
        }

        // Copia rapida bajo el candado; se dibuja ya sin el candado tomado.
        std::vector<std::string> copia;
        {
            std::lock_guard<std::mutex> guard(mutexLineas);
            copia = lineasVivas;
        }

        float y = 130.f;
        for (const auto& linea : copia) {
            sf::Text t;
            t.setFont(font);
            t.setString(linea);
            t.setCharacterSize(14);
            t.setFillColor(sf::Color(40, 40, 40));
            t.setPosition(20.f, y);
            window.draw(t);
            y += 22.f;
        }

        window.display();
    }
}
