#ifndef PAGO_H 
#define PAGO_H

#include <string>
#include <iostream>

#include "Tarjeta.h"

using namespace std;

class Pago{
	private:
		Tarjeta tarjeta;
		float monto;
		bool aprobado;
		string folioPedido;

		// datos de auditoria
		string fechaPago;
		string referencia;       // ej. "PAG-PED-..."
		string motivo;           // por que se rechazo (vacio si se aprobo)
		string usernameCliente;  // quien pago
		string idCafeteria;      // a quien se le pago

	public:
		Pago();
		
		~Pago();
		
		const Tarjeta& getTarjeta() const;
		float getMonto() const;
		bool getAprobado() const;
		string getFolioPedido() const;
		string getFechaPago() const;
		string getReferencia() const;
		string getMotivo() const;
		string getUsernameCliente() const;
		string getIdCafeteria() const;

		void asignarTarjeta(const Tarjeta& tarjeta);
		void setMonto(float m);
		void setAprobado(bool a);
		void setFolioPedido(const string& folio);
		void setFechaPago(const string& f);
		void setReferencia(const string& r);
		void setMotivo(const string& m);
		void setUsernameCliente(const string& u);
		void setIdCafeteria(const string& c);
};

#endif
		
