#ifndef PAGO_H 
#define PAGO_H

#include <string>
#include <iostream>

#include "Tarjeta.h"
#include "BaseDatos.h"

using namespace std;

class Pago{
	private:
		Tarjeta tarjeta;
		float monto;
		bool aprobado;
		string folioPedido;

	public:
		Pago();
		
		~Pago();
		
		const Tarjeta& getTarjeta() const;
		float getMonto() const;
		bool getAprobado() const;
		string getFolioPedido() const;

		void asignarTarjeta(const Tarjeta& tarjeta);
		void setMonto(float m);
		void setAprobado(bool a);
		void setFolioPedido(const string& folio);
};

#endif
		
