#ifndef SENALES_H
#define SENALES_H

#include <string>

// Manejo de senales (signals) para los programas de consola: cliente,
// cafeteria, administrador. El servidor y el simulador tienen su propio
// manejo, mas completo, dentro de Servidor.cpp / Simulador.cpp.
//
// Idea: el handler de una senal NO puede hacer casi nada seguro (no cout, no
// malloc, no mutex). Por eso solo levanta una bandera (sig_atomic_t) y el
// programa la revisa en su ciclo principal.
namespace Senales {

	// SIGINT (Ctrl+C) y SIGTERM (kill) -> levantan la bandera "interrumpido".
	// SIGPIPE -> se ignora (si el servidor se cae, send() falla con error
	// normal en vez de matar el proceso).
	// Se instala SIN SA_RESTART para que un read()/recv() bloqueado se
	// interrumpa (EINTR) y el programa pueda reaccionar de inmediato.
	void instalarCliente();

	bool interrumpido();
	void limpiar();

	// Espera hasta que el usuario presione ENTER o llegue una senal.
	// Regresa true si fue ENTER, false si fue una senal.
	bool esperarEnterOInterrupcion();

	// Lectura robusta de consola (no se cicla con EOF ni con texto no numerico).
	// Regresan false si hubo EOF / Ctrl+C.
	bool leerLinea(std::string& salida);      // quita '|' y '\r' (protegen el protocolo)
	bool leerEntero(int& salida);             // pide de nuevo si no es numero
}

#endif
