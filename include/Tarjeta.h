#ifndef TARJETA_H
#define TARJETA_H

#include <string>
#include <iostream>
#include <random>

using namespace std;

class Tarjeta{
	private:
		string numeroTarjeta;   // string: un numero de tarjeta real (16 digitos) no cabe en un int
		string CVV;             // string: preserva ceros a la izquierda (ej. "007")
		string nombrePropietario;
		string fechaVencimiento;

	public:
		Tarjeta();
		Tarjeta(string numeroTarjeta, string CVV, string nombrePropietario, string fechaVencimiento);
		~Tarjeta();

		string getNombrePropietario() const;
		string getNumeroTarjeta() const;
		string getCVV() const;
		string getFechaVencimiento() const;

		void setNombrePropietario(const string& nombre);
		void setNumeroTarjeta(const string& numero);
		void setCVV(const string& cvv);
		void setFechaVencimiento(const string& fecha);

		// "**** **** **** 1234": para mostrar/registrar sin exponer el numero completo.
		string enmascarada() const;

		// true si la fecha "MM/AA" ya paso (la tarjeta vale hasta el fin de ese mes)
		// o si el formato no se puede leer.
		bool estaVencida() const;

		// Algoritmo de Luhn: valida el digito verificador de un numero de tarjeta.
		static bool luhnValido(const string& numero);

		// Tarjeta SIMULADA para un bot: 16 digitos con prefijo 9999 (no pertenece a
		// ningun banco real), digito verificador Luhn valido, CVV de 3 digitos y
		// vencimiento aleatorio entre 2 y 5 anios a futuro.
		static Tarjeta generarSimulada(const string& titular, mt19937& generador);
};

#endif
