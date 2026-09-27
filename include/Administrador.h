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
		vector<Usuario> listaUsuarios;
		vector<Cafeteria> listaCafeterias;

		static vector<string> usuariosEnLinea;
		static mutex mutexEnLinea;

		int socketAdmin;
		string ipServidor;
		int puerto;

	public:
		Administrador();
		Administrador(string nombre, string correo, string contrasena, string username);

		~Administrador();

		bool conectar();

		const vector<Usuario>& getListaUsuarios() const;
		const vector<Cafeteria>& getListaCafeterias() const;

		// Consultas que se hacen a traves del servidor
		bool cargarUsuarios();
		bool cargarCafeterias();

		void verUsuarios();
		void verCafeterias();
		void verUsuariosEnLinea();

		static void marcarConectado(const string& username);
		static void marcarDesconectado(const string& username);

		bool iniciarSesionAdmin();

		// Para que la interfaz SFML pueda pedir informacion al servidor
		string enviarPeticion(const string& comando);

		// Consultas para el panel
		vector<string> obtenerUsuariosPanel();
		vector<string> obtenerCafeteriasPanel();
		vector<string> obtenerPedidosPanel();
		vector<string> obtenerInventarioPanel();
		vector<string> obtenerUsuariosEnLineaPanel();
};

#endif
