#ifndef TARJETA_H 
#define TARJETA_H

#include <string>
#include <iostream>

using namespace std;

class Tarjeta{
	private:
		int numeroTarjeta;
		int CVV;
		string nombrePropietario;
		string fechaVencimiento;

	public:
		Tarjeta();
		
		~Tarjeta();
		
		string getNombrePropietario();
		int getNumeroTarjeta();
		int getCVV();
		string getFechaVencimiento();
		string getUsernamePropietario();

		void setNombrePropietario();
		void setNumeroTarjeta();
		void setCVV();
		void setFechaVencimiento();
};

#endif
		
