#ifndef TARJETA_H 
#define TARJETA_H

#include <string>
#include <iostream>
#include "BaseDatos.h"

using namespace std;

class Tarjeta{
	private:
		int numeroTarjeta;
		int CVV;
		string nombrePropietario;
		string fechaVencimiento;

	public:
		Tarjeta();
	        Tarjeta(int numeroTarjeta, int CVV, string nombrePropietario, string fechaVencimiento)	
		~Tarjeta();
		
		string getNombrePropietario() const;
		int getNumeroTarjeta() const;
		int getCVV() const;
		string getFechaVencimiento() const;
		string getUsernamePropietario() const;

		void setNombrePropietario(const string& nombre);
		void setNumeroTarjeta(int numero);
		void setCVV(int cvv);
		void setFechaVencimiento(const string& fecha);
};

#endif
		
