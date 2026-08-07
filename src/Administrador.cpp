#include "Administrador.h"

using namespace std;

vector<Usuario> listaUsuarios;



Administrador::Administrador() { }
		
Administrador::~Administrador() { }

const vector<Usuario>& Administrador::getListaUsuarios() const {
	return listaUsuarios;
} 

void Administrador::setListaUsuarios() {
	listaUsuarios = obtenerUsuarios();
}

void Administrador::verLista() {
	
}

		
