#include "CodigoQR.h"

// Constructor: guarda la dirección URL y genera la textura
CodigoQR::CodigoQR(const std::string& url) : urlWeb(url) {
    construirGraficoQR();
}

void CodigoQR::construirGraficoQR() {
    // Genera la matriz lógica del QR usando la librería "QrCode.hpp"
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(urlWeb.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
   
    int size = qr.getSize();
    int border = 2; // margen de seguridad para facilitar la lectura del QR
    int fullSize = size + (border * 2);

    // Crear una imagen en RAM con fondo blanco
    sf::Image img;
    img.create(fullSize, fullSize, sf::Color::White);

    // Pintar de negro los píxeles de la matriz del QR 
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y)) {
                img.setPixel(x + border, y + border, sf::Color::Black);
            }
        }
    }

    // Copiar los píxeles de la memoria RAM a la VRAM (GPU)
    texturaQR.loadFromImage(img);

    // Sirve para mantener el QR nítido al escalar
    texturaQR.setSmooth(false);

    // Enlazar la textura al Sprite
    spriteQR.setTexture(texturaQR);

    // Definir un tamaño de 200x200 píxeles para la pantalla
    float escala = 200.0f / static_cast<float>(fullSize);
    spriteQR.setScale(escala, escala);
}

void CodigoQR::setPosicion(float x, float y) {
    spriteQR.setPosition(x, y);
}

void CodigoQR::dibujar(sf::RenderWindow& ventana, float x, float y) {
    spriteQR.setPosition(x, y);
    ventana.draw(spriteQR);
}
