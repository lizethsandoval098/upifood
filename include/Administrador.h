#ifndef ADMINISTRADOR_H
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>
#include <mutex>

#include "Usuario.h"
#include "Cafeteria.h"

using namespace std;

class Administrador : public Usuario {
	private:
		vector<Usuario> listaUsuarios;      // orden de registro (llega asi de la BD)
		vector<Cafeteria> listaCafeterias;

		// Usuarios conectados en este momento a la plataforma.
		// static + mutex porque TODOS los hilos/conexiones activas
		// escriben aqui al conectarse/desconectarse (seccion critica).
		static vector<string> usuariosEnLinea;
		static mutex mutexEnLinea;

	public:
		Administrador();
		Administrador(string nombre, string correo, string contrasena, string username);

		~Administrador();

		const vector<Usuario>& getListaUsuarios() const;
		const vector<Cafeteria>& getListaCafeterias() const;

		void cargarUsuarios();      // trae la lista de usuarios desde la BD
		void verUsuarios();         // 1) clientes en el orden en que se registraron
		void verCafeterias();       // 2) cafeterias + cantidad de pedidos de cada una
		void verUsuariosEnLinea();  // quien esta conectado ahora mismo

		// Llamados por cualquier hilo/conexion cuando un usuario entra o sale.
		static void marcarConectado(const string& username);
		static void marcarDesconectado(const string& username);
};

#endif
