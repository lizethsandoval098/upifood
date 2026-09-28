#ifndef VENTANA_H
#define VENTANA_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
// Ventana: envoltorio sencillo de sf::RenderWindow.
//
// REGLA DE ORO DE SFML: una ventana abierta necesita que le atiendas los
// eventos (pollEvent) todo el tiempo. Si la abres y luego te quedas
// esperando otra cosa (por ejemplo un cin >> ...), el sistema cree que el
// programa se congelo y te sale "Forzar salida". Por eso:
//   - El constructor NO abre la ventana (solo prepara todo).
//   - La ventana se abre con abrir() o con loop(), y desde ese momento
//     el programa debe estar corriendo SU ciclo de eventos.
// ---------------------------------------------------------------------
class Ventana {
private:
    sf::RenderWindow window;
    sf::Font font;
    bool fuenteCargada;

    std::string titulo;
    unsigned int ancho;
    unsigned int alto;

    std::vector<sf::Text> textos; // solo para la API simple (agregarTexto/loop)

    bool cargarFuente();

public:
    Ventana(const std::string& nombreVentana, int ancho, int alto);

    // ---- ciclo de vida ----
    void abrir();                 // crea la ventana en pantalla (si no estaba abierta)
    void cerrar();
    bool estaAbierta() const;

    // ---- acceso a SFML para quien arma su propio ciclo (ver mainAdministrador.cpp) ----
    sf::RenderWindow& getWindow();
    bool tieneFuente() const;

    // ---- dibujo ----
    // Dibuja texto (acepta acentos y ñ: se interpreta como UTF-8).
    void dibujarTexto(const std::string& texto, float x, float y,
                      unsigned int size = 20, sf::Color color = sf::Color::Black);

    // Dibuja el texto centrado dentro de un rectangulo (para botones).
    void dibujarTextoCentrado(const std::string& texto, const sf::FloatRect& area,
                              unsigned int size = 20, sf::Color color = sf::Color::White);

    // ---- API simple: una ventana que solo muestra textos fijos ----
    void agregarTexto(const std::string& texto, int x, int y, int size = 20);
    void limpiar();

    // Abre la ventana, muestra los textos agregados y bloquea hasta que
    // el usuario la cierra. Al volver, la ventana ya esta cerrada.
    void loop();
};

#endif
