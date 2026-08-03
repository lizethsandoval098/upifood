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
		string contra;
		string username;
		int anio;
		string escuela;

	public:
		Usuario();
		Usuario(string tipoUsuario, string nombre, string correo, string contra);	
		Usuario(string tipoUsuario, string nombre, string correo, string contra, string username, int anio, string escuela);
			
		~Usuario();

		string getNombre();
		string getTipoUsuario();
		string getCorreo();
		string getContra();

		void setNombre();
		void setTipoUsuario();
		void setCorreo();
		void setContra();

		void iniciarSesion();
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);
};

#endif
		
