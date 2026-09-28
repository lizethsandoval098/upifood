#include "Widgets.h"
#include <algorithm>

// Paleta (tonos cafe/crema, acorde a una cafeteria)
static const sf::Color CAFE_NORMAL(111, 78, 55);
static const sf::Color CAFE_HOVER(140, 102, 74);
static const sf::Color CAFE_ACTIVO(74, 50, 35);
static const sf::Color GRIS_BORDE(190, 180, 170);
static const sf::Color TEXTO_OSCURO(50, 40, 35);
static const sf::Color TEXTO_SUAVE(130, 120, 110);

// ----------------------------- Boton -----------------------------

Boton::Boton() : hover(false), activo(false) {
}

void Boton::setColor(const sf::Color& normal) {
    colorNormal = normal;
    colorHover = sf::Color(static_cast<sf::Uint8>(std::min(255, normal.r + 30)),
                           static_cast<sf::Uint8>(std::min(255, normal.g + 30)),
                           static_cast<sf::Uint8>(std::min(255, normal.b + 30)));
}

Boton::Boton(const std::string& textoBoton, float x, float y, float ancho, float alto)
    : texto(textoBoton), hover(false), activo(false) {
    forma.setSize(sf::Vector2f(ancho, alto));
    forma.setPosition(x, y);
    forma.setFillColor(colorNormal);
}

void Boton::setTexto(const std::string& t) {
    texto = t;
}

void Boton::setActivo(bool a) {
    activo = a;
}

bool Boton::contiene(float x, float y) const {
    return forma.getGlobalBounds().contains(x, y);
}

void Boton::actualizarHover(float x, float y) {
    hover = contiene(x, y);
}

void Boton::dibujar(Ventana& ventana) {
    if (activo) {
        forma.setFillColor(CAFE_ACTIVO);
    } else if (hover) {
        forma.setFillColor(colorHover);
    } else {
        forma.setFillColor(colorNormal);
    }

    ventana.getWindow().draw(forma);
    ventana.dibujarTextoCentrado(texto, forma.getGlobalBounds(), 18, sf::Color::White);
}

// --------------------------- CampoTexto ---------------------------

CampoTexto::CampoTexto() : enfocado(false), oculto(false), maxLongitud(32) {
}

CampoTexto::CampoTexto(const std::string& textoEtiqueta, float x, float y,
                       float ancho, float alto, bool esOculto)
    : etiqueta(textoEtiqueta), enfocado(false), oculto(esOculto), maxLongitud(32) {
    forma.setSize(sf::Vector2f(ancho, alto));
    forma.setPosition(x, y);
    forma.setFillColor(sf::Color::White);
    forma.setOutlineThickness(2.0f);
    forma.setOutlineColor(GRIS_BORDE);
}

void CampoTexto::setEnfocado(bool e) {
    enfocado = e;
}

bool CampoTexto::estaEnfocado() const {
    return enfocado;
}

bool CampoTexto::contiene(float x, float y) const {
    return forma.getGlobalBounds().contains(x, y);
}

void CampoTexto::recibirCaracter(sf::Uint32 unicode) {
    if (!enfocado) {
        return;
    }

    if (unicode == 8) {                    // backspace: borra UN caracter (aunque sea acentuado)
        while (!contenido.empty()) {
            unsigned char ultimo = static_cast<unsigned char>(contenido.back());
            contenido.pop_back();
            if ((ultimo & 0xC0) != 0x80) {
                break;
            }
        }
        return;
    }

    std::string bytes;

    if (unicode >= 32 && unicode < 127) {           // ASCII imprimible
        bytes += static_cast<char>(unicode);
    } else if (unicode >= 0xA0 && unicode <= 0x24F) { // letras con acento, ñ, ¿, ¡, etc. (UTF-8 de 2 bytes)
        bytes += static_cast<char>(0xC0 | (unicode >> 6));
        bytes += static_cast<char>(0x80 | (unicode & 0x3F));
    } else {
        return; // otros caracteres (emojis, control) se ignoran
    }

    if (contenido.size() + bytes.size() <= maxLongitud) {
        contenido += bytes;
    }
    // Enter (13) y Tab (9) los maneja quien usa el campo, no el campo.
}

const std::string& CampoTexto::getContenido() const {
    return contenido;
}

void CampoTexto::setContenido(const std::string& texto) {
    contenido = texto;
}

void CampoTexto::setMaxLongitud(std::size_t maximo) {
    maxLongitud = maximo;
}

void CampoTexto::limpiar() {
    contenido.clear();
}

void CampoTexto::dibujar(Ventana& ventana) {
    forma.setOutlineColor(enfocado ? CAFE_NORMAL : GRIS_BORDE);
    ventana.getWindow().draw(forma);

    sf::FloatRect r = forma.getGlobalBounds();

    // Etiqueta arriba del campo
    ventana.dibujarTexto(etiqueta, r.left, r.top - 24.0f, 16, TEXTO_SUAVE);

    // Contenido (asteriscos si es contrasena) + cursor "_" si tiene el foco
    std::string mostrado = oculto ? std::string(contenido.size(), '*') : contenido;
    if (enfocado) {
        mostrado += "_";
    }

    ventana.dibujarTexto(mostrado, r.left + 10.0f, r.top + 8.0f, 20, TEXTO_OSCURO);
}
