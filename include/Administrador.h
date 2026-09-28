#ifndef ADMINISTRADOR_H
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>

#include "Usuario.h"
#include "Cafeteria.h"

using namespace std;

// Resumen de una cafeteria tal como la manda el servidor (LISTAR_CAFETERIAS).
struct ResumenCafeteria {
	string idCafeteria;
	string nombre;
	string correo;
	string username;
	int pedidos;
};

class Administrador : public Usuario {
private:
	vector<Usuario> listaUsuarios;             // orden de registro
	vector<Cafeteria> listaCafeterias;
	vector<ResumenCafeteria> resumenCafeterias; // cafeterias + cantidad de pedidos
	vector<string> usuariosEnLinea;

	int socketAdmin;
	string ipServidor;
	int puerto;

	string ultimoError; // motivo del ultimo fallo (para mostrarlo en la ventana)

	// Manda un comando de tipo lista ("OK|N" + N lineas) y regresa las N lineas.
	bool pedirLista(const string& comando, vector<string>& lineas);

public:
	Administrador();
	Administrador(string nombre, string correo, string contrasena, string username);

	// Tiene un socket abierto: copiarlo cerraria el socket dos veces.
	Administrador(const Administrador&) = delete;
	Administrador& operator=(const Administrador&) = delete;

	~Administrador();

	void setIpServidor(const string& ip);
	bool conectar();

	const vector<Usuario>& getListaUsuarios() const;
	const vector<Cafeteria>& getListaCafeterias() const;
	const vector<ResumenCafeteria>& getResumenCafeterias() const;
	const vector<string>& getUsuariosEnLinea() const;
	string getUltimoError() const;

	// ---- Piden los datos al servidor y los guardan (regresan false si fallan) ----
	bool cargarUsuarios();
	bool cargarCafeterias();
	bool cargarUsuariosEnLinea();

	// ---- Versiones de consola: cargan y ademas imprimen ----
	void verUsuarios();
	void verCafeterias();
	void verUsuariosEnLinea();

	// Login desde la ventana (SFML): recibe los datos ya escritos.
	bool iniciarSesionAdmin(const string& username, const string& contrasena);

	// Login por consola: pregunta con cin y llama al de arriba.
	bool iniciarSesionAdmin();
};

#endif
