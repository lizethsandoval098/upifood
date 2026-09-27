#include "Administrador.h"
#include "BaseDatos.h"
#include "Protocolo.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

vector<string> Administrador::usuariosEnLinea;
mutex Administrador::mutexEnLinea;


Administrador::Administrador() {
	socketAdmin = -1;
	ipServidor = "100.91.99.27";
	puerto = 5000;
}


Administrador::Administrador(
	string nombre,
	string correo,
	string contrasena,
	string username
)
	: Usuario("Admin", nombre, correo, contrasena, username) {

	socketAdmin = -1;
	ipServidor = "100.91.99.27";
	puerto = 5000;
}


Administrador::~Administrador() {
	if(socketAdmin != -1) {
		close(socketAdmin);
		socketAdmin = -1;
	}
}


bool Administrador::conectar() {

	socketAdmin = socket(AF_INET, SOCK_STREAM, 0);

	if(socketAdmin == -1) {
		cout << "Error creando socket." << endl;
		return false;
	}

	sockaddr_in direccionServidor;

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_port = htons(puerto);

	inet_pton(
		AF_INET,
		ipServidor.c_str(),
		&direccionServidor.sin_addr
	);

	if(connect(
		socketAdmin,
		(sockaddr*)&direccionServidor,
		sizeof(direccionServidor)
	) == -1) {

		cout << "Error conectando al servidor." << endl;
		cout << "Revisa la IP/Tailscale." << endl;

		close(socketAdmin);
		socketAdmin = -1;

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


/*
    Manda cualquier comando al servidor.

    Ejemplo:

    enviarPeticion("LISTAR_USUARIOS")

    El servidor recibe el comando,
    consulta SQLite y devuelve la respuesta.
*/
string Administrador::enviarPeticion(const string& comando) {

	if(socketAdmin == -1) {
		return "ERR|Socket no conectado.";
	}

	return enviarComando(socketAdmin, comando);
}


/*
    Carga usuarios desde el SERVIDOR.
*/
bool Administrador::cargarUsuarios() {

	string respuesta = enviarPeticion("LISTAR_USUARIOS");

	vector<string> campos = separarCampos(respuesta);

	if(campos.empty()) {
		return false;
	}

	if(campos[0] != "OK") {
		return false;
	}

	listaUsuarios.clear();

	/*
	    La respuesta tendrá:

	    OK|cantidad

	    En esta primera versión el servidor
	    devolverá todos los usuarios separados
	    por registros.
	*/

	if(campos.size() < 2) {
		return false;
	}

	int cantidad = stoi(campos[1]);

	for(int i = 0; i < cantidad; i++) {

		string registro;

		if(!recibirMensaje(socketAdmin, registro)) {
			return false;
		}

		vector<string> datos = separarCampos(registro);

		if(datos.size() < 5) {
			continue;
		}

		Usuario usuario(
			datos[0],
			datos[1],
			datos[2],
			"",
			datos[4]
		);

		usuario.setContrasenaHash(datos[3]);

		listaUsuarios.push_back(usuario);
	}

	return true;
}


/*
    Carga cafeterias desde el SERVIDOR.
*/
bool Administrador::cargarCafeterias() {

	string respuesta = enviarPeticion("LISTAR_CAFETERIAS");

	vector<string> campos = separarCampos(respuesta);

	if(campos.empty()) {
		return false;
	}

	if(campos[0] != "OK") {
		return false;
	}

	if(campos.size() < 2) {
		return false;
	}

	listaCafeterias.clear();

	int cantidad = stoi(campos[1]);

	for(int i = 0; i < cantidad; i++) {

		string registro;

		if(!recibirMensaje(socketAdmin, registro)) {
			return false;
		}

		vector<string> datos = separarCampos(registro);

		if(datos.size() < 4) {
			continue;
		}

		Cafeteria cafeteria(
			datos[0],
			datos[1],
			"",
			datos[2],
			datos[3]
		);

		listaCafeterias.push_back(cafeteria);
	}

	return true;
}


void Administrador::verUsuarios() {

	if(listaUsuarios.empty()) {

		cout << "No hay usuarios registrados." << endl;
		return;
	}

	for(size_t i = 0; i < listaUsuarios.size(); i++) {

		cout << "Nombre: "
		     << listaUsuarios[i].getNombre()
		     << endl;

		cout << "Username: "
		     << listaUsuarios[i].getUsername()
		     << endl;

		cout << "Correo: "
		     << listaUsuarios[i].getCorreo()
		     << endl;

		cout << "Tipo de usuario: "
		     << listaUsuarios[i].getTipoUsuario()
		     << endl;

		cout << "----------------------------------"
		     << endl;
	}
}


void Administrador::verCafeterias() {

	if(listaCafeterias.empty()) {

		cout << "No hay cafeterias registradas."
		     << endl;

		return;
	}

	for(const auto& cafeteria : listaCafeterias) {

		cout << "Cafeteria: "
		     << cafeteria.getNombreCafeteria()
		     << endl;

		cout << "ID: "
		     << cafeteria.getIdCafeteria()
		     << endl;

		cout << "Username: "
		     << cafeteria.getUsername()
		     << endl;

		cout << "----------------------------------"
		     << endl;
	}
}


void Administrador::verUsuariosEnLinea() {

	lock_guard<mutex> guard(mutexEnLinea);

	if(usuariosEnLinea.empty()) {

		cout << "No hay usuarios conectados."
		     << endl;

		return;
	}

	cout << "Usuarios en linea:"
	     << endl;

	for(const auto& username : usuariosEnLinea) {

		cout << " - "
		     << username
		     << endl;
	}
}


void Administrador::marcarConectado(
	const string& username
) {

	lock_guard<mutex> guard(mutexEnLinea);

	for(const auto& usuario : usuariosEnLinea) {

		if(usuario == username) {
			return;
		}
	}

	usuariosEnLinea.push_back(username);
}


void Administrador::marcarDesconectado(
	const string& username
) {

	lock_guard<mutex> guard(mutexEnLinea);

	for(auto it = usuariosEnLinea.begin();
	    it != usuariosEnLinea.end();
	    ++it) {

		if(*it == username) {

			usuariosEnLinea.erase(it);
			return;
		}
	}
}


bool Administrador::iniciarSesionAdmin() {

	cout << "==================================" << endl;
	cout << "Inicio de sesion (Administrador)" << endl;

	string username;
	string contrasena;

	cout << "Ingrese username: ";
	cin >> username;

	cout << "Ingrese contrasena: ";
	cin >> contrasena;

	string respuesta =
		enviarPeticion(
			"LOGIN_ADMIN|" +
			username +
			"|" +
			contrasena
		);

	vector<string> campos =
		separarCampos(respuesta);

	if(campos.empty() || campos[0] != "OK") {

		string motivo =
			(campos.size() > 1)
			? campos[1]
			: "Error desconocido.";

		cout << motivo << endl;

		return false;
	}

	if(campos.size() < 4) {

		cout << "Respuesta invalida del servidor."
		     << endl;

		return false;
	}

	setNombre(campos[1]);
	setCorreo(campos[2]);
	setUsername(campos[3]);
	setTipoUsuario("Admin");

	cout << "Inicio de sesion correcto."
	     << endl;

	return true;
}


/*
    Estas funciones se utilizan posteriormente
    para las diferentes pantallas del panel.
*/

vector<string> Administrador::obtenerUsuariosPanel() {

	vector<string> datos;

	if(!cargarUsuarios()) {

		datos.push_back("No se pudieron cargar los usuarios.");

		return datos;
	}

	if(listaUsuarios.empty()) {

		datos.push_back("No hay usuarios registrados.");

		return datos;
	}

	for(const auto& usuario : listaUsuarios) {

		string linea =
			usuario.getNombre() +
			" | " +
			usuario.getUsername() +
			" | " +
			usuario.getCorreo() +
			" | " +
			usuario.getTipoUsuario();

		datos.push_back(linea);
	}

	return datos;
}


vector<string> Administrador::obtenerCafeteriasPanel() {

	vector<string> datos;

	if(!cargarCafeterias()) {

		datos.push_back(
			"No se pudieron cargar las cafeterias."
		);

		return datos;
	}

	if(listaCafeterias.empty()) {

		datos.push_back(
			"No hay cafeterias registradas."
		);

		return datos;
	}

	for(const auto& cafeteria : listaCafeterias) {

		string linea =
			cafeteria.getNombreCafeteria() +
			" | ID: " +
			cafeteria.getIdCafeteria() +
			" | " +
			cafeteria.getUsername();

		datos.push_back(linea);
	}

	return datos;
}


vector<string> Administrador::obtenerPedidosPanel() {

	vector<string> datos;

	string respuesta =
		enviarPeticion("LISTAR_PEDIDOS");

	vector<string> campos =
		separarCampos(respuesta);

	if(campos.empty() || campos[0] != "OK") {

		datos.push_back("No se pudieron cargar los pedidos.");

		return datos;
	}

	int cantidad = stoi(campos[1]);

	for(int i = 0; i < cantidad; i++) {

		string registro;

		if(!recibirMensaje(socketAdmin, registro)) {
			break;
		}

		datos.push_back(registro);
	}

	if(datos.empty()) {

		datos.push_back(
			"No hay pedidos registrados."
		);
	}

	return datos;
}


vector<string> Administrador::obtenerInventarioPanel() {

	vector<string> datos;

	string respuesta =
		enviarPeticion("LISTAR_INVENTARIO");

	vector<string> campos =
		separarCampos(respuesta);

	if(campos.empty() || campos[0] != "OK") {

		datos.push_back(
			"No se pudo cargar el inventario."
		);

		return datos;
	}

	int cantidad = stoi(campos[1]);

	for(int i = 0; i < cantidad; i++) {

		string registro;

		if(!recibirMensaje(socketAdmin, registro)) {
			break;
		}

		datos.push_back(registro);
	}

	if(datos.empty()) {

		datos.push_back(
			"No hay productos registrados."
		);
	}

	return datos;
}


vector<string> Administrador::obtenerUsuariosEnLineaPanel() {

	vector<string> datos;

	/*
	    Esta parte posteriormente debe consultar
	    la lista real del SERVIDOR.

	    Por ahora usamos la lista local.
	*/

	lock_guard<mutex> guard(mutexEnLinea);

	if(usuariosEnLinea.empty()) {

		datos.push_back(
			"No hay usuarios conectados."
		);

		return datos;
	}

	for(const auto& usuario : usuariosEnLinea) {

		datos.push_back(usuario);
	}

	return datos;
}
