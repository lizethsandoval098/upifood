#include "Administrador.h"

using namespace std;

Administrador::Administrador() { }
		
Administrador::~Administrador() { }

const vector<Usuario>& Administrador::getListaUsuarios() const {
	return listaUsuarios;
} 

//Se crea un objeto de la clase BaseDatos y se utiliza el metodo de conectar el cual va a conectar a la base de datos y si se realiza correctamente la conexion con el vector que se 
//creo arriba se almacenarar la lista de usuarios que se obtendra con obtenerUsuario y una vez obtenida se desconecta de la base de datos ya que ya obtuvo los datos que necesitaba. 
void Administrador::setListaUsuarios() {

	BaseDatos db;

	if(db.conectar()){

		listaUsuarios = db.obtenerUsuarios();

		db.desconectar();

	}
}

void Administrador::verLista() {
//Verifica que la lista de usuarios no este vacia ya que size dice la cantidad exacta de datos que contiene el vector.
    if(listaUsuarios.size() == 0){

        cout << "No hay usuarios registrados." << endl;

        return;
    }
//Si el vector no esta vacio entonces comenzara a recorrer usuario por usuario e ir obteniendo sus datos 
    for(int i = 0; i < listaUsuarios.size(); i++){

        cout << "Nombre: " << listaUsuarios[i].getNombre() << endl;
        cout << "Username: " << listaUsuarios[i].getUsername() << endl;
        cout << "Correo: " << listaUsuarios[i].getCorreo() << endl;
        cout << "Tipo de usuario: " << listaUsuarios[i].getTipoUsuario() << endl;

        cout << "----------------------------------" << endl;
    }
}






//void Administrador::setListaUsuarios() {
//	listaUsuarios = obtenerUsuarios();
//}

//void Administrador::verLista() {
	
//}

		
