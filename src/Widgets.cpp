#include "Widgets.h"

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

Boton::Boton(const std::string& textoBoton, float x, float y, float ancho, float alto)
    : texto(textoBoton), hover(false), activo(false) {
    forma.setSize(sf::Vector2f(ancho, alto));
    forma.setPosition(x, y);
    forma.setFillColor(CAFE_NORMAL);
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
        forma.setFillColor(CAFE_HOVER);
    } else {
        forma.setFillColor(CAFE_NORMAL);
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

    if (unicode == 8) {                    // backspace
        if (!contenido.empty()) {
            contenido.pop_back();
        }
    } else if (unicode >= 32 && unicode < 127) { // caracteres ASCII imprimibles
        if (contenido.size() < maxLongitud) {
            contenido += static_cast<char>(unicode);
        }
    }
    // Enter (13) y Tab (9) los maneja quien usa el campo, no el campo.
}

const std::string& CampoTexto::getContenido() const {
    return contenido;
}

void CampoTexto::setContenido(const std::string& texto) {
    contenido = texto.substr(0, maxLongitud);
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
