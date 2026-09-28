#ifndef USUARIO_H
#define USUARIO_H

#include <string>
#include <iostream>

using namespace std;

// Clase base de la que derivan Administrador, Cafeteria y Cliente.
// Solo contiene lo que TODOS los tipos de usuario tienen en comun.
class Usuario{
	protected:
		string nombre;
		string correo;
		string tipoUsuario;   // "Admin", "Cafe" o "Cliente" (debe coincidir con el CHECK de la tabla Usuarios)
		string contrasena;
		string username;

	public:
		Usuario();
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena);
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena, string username);

		virtual ~Usuario();

		string getNombre() const;
		string getTipoUsuario() const;
		string getCorreo() const;
		string getContrasena() const;
		string getUsername() const;

		void setNombre(const string& n);
		void setTipoUsuario(const string& tu);
		void setCorreo(const string& c);
		void setUsername(const string& u);

		void setContrasena(const string& c);
		bool verificarContrasena(const string& intento) const;
};

#endif
