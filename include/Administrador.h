#ifndef ADMINISTRADOR_H 
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>
#include "Usuario.h"

using namespace std;

class Administrador{
	private:
		vector<Usuario> listaUsuarios;

	public:
		Administrador();
		
		~Administrador();

		void cargarLista();
		void verLista();
};

#endif
		
