#include "Ventana.h"
#include <iostream>

// Escrito para SFML 2.5 / 2.6 (la de apt en Ubuntu). Si tienen SFML 3.x
// la sintaxis de sf::Text, sf::Event y sf::VideoMode cambia.

Ventana::Ventana(const std::string& nombreVentana, int anchoVentana, int altoVentana)
    : fuenteCargada(false), titulo(nombreVentana),
      ancho(static_cast<unsigned int>(anchoVentana)),
      alto(static_cast<unsigned int>(altoVentana)) {

    fuenteCargada = cargarFuente();

    if (!fuenteCargada) {
        std::cout << "[Ventana] AVISO: no se encontro ninguna fuente .ttf. "
                  << "Se veran los recuadros pero NO el texto. "
                  << "Copia una fuente a recursos/font.ttf." << std::endl;
    }
}

// Prueba varias rutas para no depender de desde que carpeta se ejecuta el
// programa (desde la raiz, desde build/, etc.).
bool Ventana::cargarFuente() {
    std::vector<std::string> candidatas;

#ifdef RECURSOS_DIR
    candidatas.push_back(std::string(RECURSOS_DIR) + "/font.ttf"); // ruta absoluta desde CMake
#endif
    candidatas.push_back("recursos/font.ttf");
    candidatas.push_back("../recursos/font.ttf");
    candidatas.push_back("../../recursos/font.ttf");

    // Fuentes que suelen traer las distribuciones Linux, por si acaso.
    candidatas.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidatas.push_back("/usr/share/fonts/TTF/DejaVuSans.ttf");
    candidatas.push_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    candidatas.push_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");

    for (const std::string& ruta : candidatas) {
        if (font.loadFromFile(ruta)) {
            return true;
        }
    }

    return false;
}

void Ventana::abrir() {
    if (window.isOpen()) {
        return;
    }

    window.create(sf::VideoMode(ancho, alto), titulo,
                  sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60); // evita gastar el 100% del procesador
}

void Ventana::cerrar() {
    if (window.isOpen()) {
        window.close();
    }
}

bool Ventana::estaAbierta() const {
    return window.isOpen();
}

sf::RenderWindow& Ventana::getWindow() {
    return window;
}

bool Ventana::tieneFuente() const {
    return fuenteCargada;
}

void Ventana::dibujarTexto(const std::string& texto, float x, float y,
                           unsigned int size, sf::Color color) {
    if (!fuenteCargada) {
        return; // sin fuente no se puede dibujar texto (evita cerrarse de golpe)
    }

    sf::Text t;
    t.setFont(font);
    t.setString(sf::String::fromUtf8(texto.begin(), texto.end()));
    t.setCharacterSize(size);
    t.setFillColor(color);
    t.setPosition(x, y);

    window.draw(t);
}

void Ventana::dibujarTextoCentrado(const std::string& texto, const sf::FloatRect& area,
                                   unsigned int size, sf::Color color) {
    if (!fuenteCargada) {
        return;
    }

    sf::Text t;
    t.setFont(font);
    t.setString(sf::String::fromUtf8(texto.begin(), texto.end()));
    t.setCharacterSize(size);
    t.setFillColor(color);

    // Truco para centrar: mover el "origen" del texto a su centro y luego
    // ponerlo en el centro del rectangulo.
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.0f, b.top + b.height / 2.0f);
    t.setPosition(area.left + area.width / 2.0f, area.top + area.height / 2.0f);

    window.draw(t);
}

void Ventana::agregarTexto(const std::string& texto, int x, int y, int size) {
    sf::Text t;
    t.setFont(font);
    t.setString(sf::String::fromUtf8(texto.begin(), texto.end()));
    t.setCharacterSize(static_cast<unsigned int>(size));
    t.setFillColor(sf::Color::Black);
    t.setPosition(static_cast<float>(x), static_cast<float>(y));

    textos.push_back(t);
}

void Ventana::limpiar() {
    textos.clear();
}

void Ventana::loop() {
    abrir();

    while (window.isOpen()) {
        sf::Event evento;

        while (window.pollEvent(evento)) {
            if (evento.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color(245, 245, 245));

        if (fuenteCargada) {
            for (const auto& t : textos) {
                window.draw(t);
            }
        }

        window.display();
    }
}
