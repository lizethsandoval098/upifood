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

		string getNombre();
		string getTipoUsuario();
		string getCorreo();
		string getContrasena();
		string getUsername();

		void setNombre();
		void setTipoUsuario();
		void setCorreo();
		void setContrasena();

		void iniciarSesion();
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);
};

#endif
		
