#include "Administrador.h"
#include "BaseDatos.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

// Definicion de los estaticos declarados en el header.
vector<string> Administrador::usuariosEnLinea;
mutex Administrador::mutexEnLinea;

Administrador::Administrador() {
	socketAdmin = -1;
	ipServidor = "100.91.99.27";
	puerto = 5000;
}

Administrador::Administrador(string nombre, string correo, string contrasena, string username)
	: Usuario("Admin", nombre, correo, contrasena, username) {
	socketAdmin = -1;
	ipServidor = "100.91.99.27";
	puerto = 5000;
}

Administrador::~Administrador() { }

bool Administrador::conectar() {
	socketAdmin = socket(AF_INET, SOCK_STREAM, 0);

	if (socketAdmin == -1) {
		cout << "Error creando socket." << endl;
		return false;
	}

	sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_port = htons(puerto);
	inet_pton(AF_INET, ipServidor.c_str(), &direccionServidor.sin_addr);

	if (connect(socketAdmin, (sockaddr*)&direccionServidor, sizeof(direccionServidor)) == -1) {
		cout << "Error conectando al servidor. Revisa la IP/Tailscale." << endl;
		return false;
	}

	cout << "Conectado al servidor correctamente." << endl;
	return true;
}

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

bool Administrador::iniciarSesionAdmin() {
	cout << "==================================" << endl;
	cout << "Inicio de sesion (Administrador)" << endl;

	string username, contrasena;

	cout << "Ingrese username: ";
	cin >> username;

	cout << "Ingrese contrasena: ";
	cin >> contrasena;

	string respuesta = enviarComando(socketAdmin, "LOGIN_ADMIN|" + username + "|" + contrasena);
	vector<string> campos = separarCampos(respuesta);

	if (campos.empty() || campos[0] != "OK") {
		string motivo = (campos.size() > 1) ? campos[1] : "Error desconocido.";
		cout << motivo << endl;
		return false;
	}

	// Respuesta esperada: OK|nombre|correo|username
	if (campos.size() < 4) {
		cout << "Respuesta invalida del servidor." << endl;
		return false;
	}

	setNombre(campos[1]);
	setCorreo(campos[2]);
	setUsername(campos[3]);
	setTipoUsuario("Admin");

	cout << "Inicio de sesion correcto." << endl;
	return true;
}
