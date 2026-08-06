#ifndef USUARIO_H 
#define USUARIO_H

#include <string>
#include <iostream>

using namespace std;

class Usuario{
	private:
		string nombre;
		string correo;
		string tipoUsuario;
		string contrasena;
		string username;
		int anio;
		string escuela;

	public:
		Usuario();
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena);	
		Usuario(string tipoUsuario, string nombre, string correo, string contrasena, string username, int anio, string escuela);
			
		~Usuario();

		string getNombre() const;
		string getTipoUsuario() const;
		string getCorreo() const;
		string getContrasena() const;
		string getUsername() const;

		void setNombre(const string& n);
		void setTipoUsuario(const string& tu);
		void setCorreo(const string& c);
		void setContrasena(const string& c);
		void setUsername(const string& u);

		void iniciarSesion();
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);
};

#endif
		
