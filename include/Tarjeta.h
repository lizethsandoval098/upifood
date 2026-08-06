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
		
		string getNombrePropietario() const;
		int getNumeroTarjeta() const;
		int getCVV() const;
		string getFechaVencimiento() const;
		string getUsernamePropietario() const;

		void setNombrePropietario();
		void setNumeroTarjeta();
		void setCVV();
		void setFechaVencimiento();
};

#endif
		
