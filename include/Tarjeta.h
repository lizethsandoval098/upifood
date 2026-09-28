#ifndef TARJETA_H
#define TARJETA_H

#include <string>
#include <iostream>

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
};

#endif
