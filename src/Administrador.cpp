#include "Administrador.h"

using namespace std;

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

		
