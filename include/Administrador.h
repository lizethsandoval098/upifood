#ifndef ADMINISTRADOR_H 
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>
#include "Usuario.h"
#include "BaseDatos.h"

using namespace std;

class Administrador{
	private:
		vector<Usuario> listaUsuarios;

	public:
		Administrador();
		
		~Administrador();

		const vector<Usuario>& getListaUsuarios() const;
		void setListaUsuarios(const vector<Usuario>& lista);
		void cargarLista();
		void verLista();
};

#endif
		
