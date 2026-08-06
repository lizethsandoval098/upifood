#ifndef CODIGOQR_H 
#define CODIGOQR_H

#include <string>
#include <SFML/Graphics.hpp>
#include "QrCode.hpp"     // generador de qr

using namespace std;

class CodidoQR{
	private:
		string url;
		bool esValido;

		sf::Texture textura; // arma la imagen
		sf::Sprite spriteQR; // mostrar en la ventana

		void construirGraficoQR(); // genera los pixeles y prepara el Sprite

	public:
		CodigoQR();

		~CodigoQR();

		// constructor que recibe la url
		explicit CodigoQR(const string& url);

		// dibuja el codigo QR en la ventana
		void dibujar(sf::RnderWindow& ventana, float x, float y);

		// cambio de posicion del Sprite
		void setPosicion(float x, float y);

		// consultar url guardada
		string getUrl() const;

		void setUrl();

		bool getEsValido();
		void setEsValido();

		// manipular Sprite desde SFML
		sf::Sprite getSprite() const;

};

#endif
		
