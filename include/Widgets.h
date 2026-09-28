#ifndef WIDGETS_H
#define WIDGETS_H

#include <SFML/Graphics.hpp>
#include <string>
#include "Ventana.h"

// ---------------------------------------------------------------------
// Componentes de interfaz reutilizables (para admin, cafeteria y cliente).
// SFML NO trae botones ni campos de texto: solo dibuja formas y texto.
// Por eso aqui los armamos: un rectangulo + un texto + logica de "el mouse
// esta encima" / "me hicieron clic" / "estoy recibiendo teclas".
// ---------------------------------------------------------------------

class Boton {
private:
    sf::RectangleShape forma;
    std::string texto;
    bool hover;   // el mouse esta encima
    bool activo;  // es la "pestana" seleccionada actualmente
    sf::Color colorNormal = sf::Color(111, 78, 55);
    sf::Color colorHover = sf::Color(140, 102, 74);

public:
    Boton();
    Boton(const std::string& texto, float x, float y, float ancho, float alto);

    void setTexto(const std::string& t);
    void setActivo(bool a);

    // Cambia el color del boton (el color al pasar el mouse se calcula solo).
    void setColor(const sf::Color& normal);

    // Coordenadas del mouse en pixeles de la ventana.
    bool contiene(float x, float y) const;
    void actualizarHover(float x, float y);

    void dibujar(Ventana& ventana);
};

class CampoTexto {
private:
    sf::RectangleShape forma;
    std::string etiqueta;
    std::string contenido;
    bool enfocado;
    bool oculto;          // true = muestra ****** (contrasena)
    std::size_t maxLongitud;

public:
    CampoTexto();
    CampoTexto(const std::string& etiqueta, float x, float y, float ancho, float alto,
               bool oculto = false);

    void setEnfocado(bool e);
    bool estaEnfocado() const;
    bool contiene(float x, float y) const;

    // Llamar con event.text.unicode cuando el evento sea TextEntered.
    void recibirCaracter(sf::Uint32 unicode);

    const std::string& getContenido() const;
    void setContenido(const std::string& texto); // para rellenar el campo desde el programa
    void setMaxLongitud(std::size_t maximo);
    void limpiar();

    void dibujar(Ventana& ventana);
};

#endif
