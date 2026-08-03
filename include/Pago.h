#ifndef PAGO_H 
#define PAGO_H

#include <string>
#include <iostream>

using namespace std;

class Pago{
	private:
		int numeroTarjeta;
		int CVV;
		string nombrePropietario;
		string fechaVencimiento;
		float monto;
		bool aprobado;

	public:
		Pago();
		
		~Pago();
		
		string getNombrePropietario();
		int getNumeroTarjeta();
		int getCVV();
		string getFechaVencimiento();
		float getMonto();
		bool getAprobado();

		void setNombrePropietario();
		void setNumeroTarjeta();
		void setCVV();
		void setFechaVencimiento();
		void setMonto();
		void setAprobado();

		void pagar();
};

#endif
		
