#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdlib>

// ---------------------------------------------------------------------
// IP del servidor
//
// Es la IP de TAILSCALE de la computadora donde corre el programa
// `servidor` (la que tiene el archivo upiifood.db). Da igual quien creo
// la red de Tailscale: lo unico que importa es que TODAS apunten a la
// misma IP, la de la compu que corre el servidor.
//
// Para ver esa IP, en la compu del servidor:   tailscale ip -4
//
// Hay 3 formas de decirle la IP a cada programa (en este orden de prioridad):
//   1) como argumento:        ./administrador 100.64.10.5
//   2) variable de entorno:   UPIIFOOD_IP=100.64.10.5 ./administrador
//   3) la de por defecto de aqui abajo (cambienla UNA vez y ya)
// Asi nadie tiene que editar el codigo cada vez que cambia la IP.
// ---------------------------------------------------------------------
#define IP_SERVIDOR_POR_DEFECTO "100.70.231.3"
#define PUERTO_SERVIDOR_POR_DEFECTO 5000

inline std::string resolverIpServidor(int argc, char* argv[]) {
	if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0') {
		return argv[1];
	}

	const char* deEntorno = std::getenv("UPIIFOOD_IP");
	if (deEntorno != nullptr && deEntorno[0] != '\0') {
		return deEntorno;
	}

	return IP_SERVIDOR_POR_DEFECTO;
}

#endif
