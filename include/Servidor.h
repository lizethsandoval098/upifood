#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <string>
#include <mutex>

#include "BaseDatos.h"

using namespace std;

class Servidor {

	private:
		int socketServidor;
		int puerto;

		// UNA sola conexion a la base de datos, compartida por todos los
		// hilos. sqlite3 no es seguro para que varios hilos escriban al
		// mismo tiempo sin control, por eso cada operacion sobre "db" va
		// protegida con "dbMutex" (seccion critica).
		BaseDatos db;
		mutex dbMutex;

		// Atiende a UN cliente ya conectado: lee comandos en bucle, los
		// despacha y contesta, hasta que el cliente se desconecta.
		// Corre en su propio hilo (uno distinto por cada cliente conectado).
		void atenderCliente(int socketCliente);

		// Interpreta un comando ("LOGIN_CLIENTE|user|pass", etc.) y regresa
		// la respuesta ya lista para mandar ("OK|..." o "ERR|...").
		string procesarComando(const string& comando);

	public:
		Servidor();

		bool iniciar();
};

#endif
