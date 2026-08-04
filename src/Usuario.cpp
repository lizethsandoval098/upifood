#include "Usuario.h"

Usuario::Usuario(){ }

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra)
                : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra} {
}	
		
Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra, string username, int anio, string escuela) : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra}, username{username}, anio{anio}, escuela{escuela} {
}
			
Usuario::~Usuario(){ }

string Usuario::getNombre(){
	return nombre;
}

string Usuario::getTipoUsuario(){
	return tipoUsuario;
}

string Usuario::getCorreo(){
	return correo;
}

string Usuario::getContra(){
	return contra;
}

void Usuario::setNombre(string nom){
	nombre = nom;
}

void Usuario::setTipoUsuario(string tipou){
	tipoUsuario = tipou;
}

void Usuario::setCorreo(string cor){
	correo = cor;
}
void Usuario::setContra(string con){
	contra = con;
}
		
void Usuario::iniciarSesion(){
	
}
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);

