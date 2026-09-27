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
		string contrasenaHash;
		string username;

	public:
		Usuario();
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena);
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena, string username);

		virtual ~Usuario();

		string getNombre() const;
		string getTipoUsuario() const;
		string getCorreo() const;
		string getContrasenaHash() const;
		string getUsername() const;

		void setNombre(const string& n);
		void setTipoUsuario(const string& tu);
		void setCorreo(const string& c);
		void setUsername(const string& u);

		// Guarda la contrasena ya "hasheada" (nunca en texto plano).
		void setContrasena(const string& contrasenaPlano);

		// Asigna un hash YA calculado directamente (uso interno: solo lo debe
		// llamar BaseDatos al leer un usuario que ya trae su hash guardado,
		// para no volver a hashear un hash).
		void setContrasenaHash(const string& hashYaCalculado);

		// Compara una contrasena en texto plano (la que teclea el usuario)
		// contra el hash guardado. Es la unica forma de validar login.
		bool verificarContrasena(const string& intento) const;

	protected:
		// Hash simple (NO es criptograficamente seguro). Sirve para dejar de
		// guardar contrasenas en texto plano en la BD sin agregar una libreria
		// externa (OpenSSL/bcrypt) al proyecto. Para un proyecto real se
		// recomienda sustituir esto por bcrypt o Argon2.
		static string hashContrasena(const string& contrasenaPlano);
};

#endif
