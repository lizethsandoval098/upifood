#include "Administrador.h"
#include "BaseDatos.h"

using namespace std;

// Definicion de los estaticos declarados en el header.
vector<string> Administrador::usuariosEnLinea;
mutex Administrador::mutexEnLinea;

Administrador::Administrador() { }

Administrador::Administrador(string nombre, string correo, string contrasena, string username)
	: Usuario("Admin", nombre, correo, contrasena, username) {
}

Administrador::~Administrador() { }

const vector<Usuario>& Administrador::getListaUsuarios() const {
	return listaUsuarios;
}

const vector<Cafeteria>& Administrador::getListaCafeterias() const {
	return listaCafeterias;
}

// Se conecta a la BD y trae la lista de usuarios (ya viene ordenada por
// orden de registro desde BaseDatos::obtenerUsuarios).
void Administrador::cargarUsuarios() {
	BaseDatos db;

	if(db.conectar()){
		listaUsuarios = db.obtenerUsuarios();
		db.desconectar();
	}
}

void Administrador::verUsuarios() {
	if(listaUsuarios.empty()){
		cout << "No hay usuarios registrados." << endl;
		return;
	}

	for(size_t i = 0; i < listaUsuarios.size(); i++){
		cout << "Nombre: " << listaUsuarios[i].getNombre() << endl;
		cout << "Username: " << listaUsuarios[i].getUsername() << endl;
		cout << "Correo: " << listaUsuarios[i].getCorreo() << endl;
		cout << "Tipo de usuario: " << listaUsuarios[i].getTipoUsuario() << endl;
		cout << "----------------------------------" << endl;
	}
}

void Administrador::verCafeterias() {
	BaseDatos db;

	if(db.conectar()){
		listaCafeterias = db.obtenerCafeterias();
		db.desconectar();
	}

	if(listaCafeterias.empty()){
		cout << "No hay cafeterias registradas." << endl;
		return;
	}

	for(const auto& c : listaCafeterias){
		cout << "Cafeteria [" << c.getIdCafeteria() << "] " << c.getNombreCafeteria()
		     << " -> " << c.getListaPedidos().size() << " pedidos" << endl;
	}
}

void Administrador::verUsuariosEnLinea() {
	lock_guard<mutex> guard(mutexEnLinea);

	if(usuariosEnLinea.empty()){
		cout << "No hay usuarios conectados en este momento." << endl;
		return;
	}

	cout << "Usuarios en linea (" << usuariosEnLinea.size() << "):" << endl;
	for(const auto& username : usuariosEnLinea){
		cout << " - " << username << endl;
	}
}

void Administrador::marcarConectado(const string& username) {
	lock_guard<mutex> guard(mutexEnLinea);

	for(const auto& u : usuariosEnLinea){
		if(u == username) return; // ya estaba marcado
	}

	usuariosEnLinea.push_back(username);
}

void Administrador::marcarDesconectado(const string& username) {
	lock_guard<mutex> guard(mutexEnLinea);

	for(auto it = usuariosEnLinea.begin(); it != usuariosEnLinea.end(); ++it){
		if(*it == username){
			usuariosEnLinea.erase(it);
			return;
		}
	}
}
