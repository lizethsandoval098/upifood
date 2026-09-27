#include "Usuario.h"
#include <algorithm>
#include <cctype>

#include "BaseDatos.h"

Usuario::Usuario(){ }

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra)
                : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra} {
}	
		
Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra, string username, int anio, string escuela) : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra}, username{username}, anio{anio}, escuela{escuela} {
}
			
Usuario::~Usuario(){ }

string Usuario::getNombre()const{
	return nombre;
}

string Usuario::getTipoUsuario()const{
	return tipoUsuario;
}

string Usuario::getCorreo()const{
	return correo;
}

string Usuario::getContra()const{
	return contra;
}

void Usuario::setNombre(const string& nom){
	nombre = nom;
}

void Usuario::setTipoUsuario(const string& tipou){
	tipoUsuario = tipou;
}

void Usuario::setCorreo(const string& cor){
	correo = cor;
}
void Usuario::setContra(const string& con){
	contra = con;
}
		
void Usuario::iniciarSesion(){
	cout << "ingresa tu nombre de usuario: ";
		
}
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);		

void Usuario::registro()
{
	string nom, apellidoP, apellidoM, anio, escuela, correo;

	cout << "==================================" << endl;
    	cout << "Registro de usuario" << endl;

    	cout << "Ingrese su correo: ";
    	cin >> correo;

	cout << "Ingrese su nombre SIN APELLIDOS: ";
    	cin >> nom;

	cout << "Ingrese su apellido paterno: ";
    	cin >> apellidoP;

	cout << "Ingrese su apellido materno: ";
    	cin >> apellidoM;

	cout << "Ingrese el año en el que se unió al IPN: ";
    	cin >> anio;

	cout << "Ingrese el nombre de la escuela de su procedencia: ";
    	cin >> escuela;
    	
	if(validarIPN(correo, anio, escuela))
    	{
        	string usernameAsignado;
		string contra;
		username = asignarUsername(correo);

        	cout << "Registro realizado correctamente." << endl;
        	cout << "Username asignado: " << usernameAsignado << endl;
		cout << "Ingrese su contraseña: ";
		cin >> contra;

		string tipoCliente;
		
		if(normalizarTexto(escuela) == "UPIITA") {
			tipoCliente = "UPIITA";
		}
		Cliente cliente(correo, usernameAsignado, nom, contra, tipoCliente);
		
		// lo guardamos en la DB
		bd.guardarUsuarioCliente(cliente);
    	}
    	else
    	{
        	cout << "No es posible registrar el usuario." << endl;
        	cout << "El correo institucional no es valido." << endl;
    	}
}

bool Usuario::iniciarSesion()
{
    cout << "==================================" << endl;
    cout << "Inicio de sesion" << endl;

    string username;
    string contrasena;

    cout << "Ingrese username: ";
    cin >> username;

    cout << "Ingrese contrasena: ";
    cin >> contrasena;

    BaseDatos db;

    Cliente cliente = db.obtenerUsuarioCliente(username);

    if(cliente == NULL)
    {
        cout << "Usuario inexistente." << endl;
        return false;
    }

    if(contrasena != cliente.getContrasena())
       cout << "Contrasena incorrecta." << endl;
    	return false;
    }

    cout << "Inicio de sesion correcto." << endl;
    return true;
}

//Para que ese pueda validar si el usuario es del instituto o no se requiere comparar su correo con sus datos de año escuela (TALVEZ ERA CON EL NOMBRE IGUAL)
bool Usuario::validarIPN(string correo, int anio, string escuela)
{
    if(correo.empty()){
        return false;
    }

    if(anio <= 2014){ // 12 maximo para terminar una carrera / ser alumno vigente 
        return false;
    }

    if(!validarEscuela(escuela)){ 
        return false;
    }

    // Generar el username esperado
    string usuarioEsperado;
    string dominioEsperado = "@alumno.ipn.mx";
    size_t posArroba = correo.find('@');

    if(posArroba == string::npos) {
	    return false;
    }

    string dominioIngresado = correo.substr(posArroba);
    
    if(dominioIngresado != dominioEsperado) {
	    return false;
    }

    usuarioEsperado += nombre[0];
    usuarioEsperado += apellidoPaterno;
    usuarioEsperado += apellidoMaterno[0];
    usuarioEsperado += to_string(anio).substr(2,2);

    string usuarioIngresado = correo.substr(0, posArroba);

    if(usuarioIngresado.rfind(usuarioEsperado,0) == 0 && 
		    usuarioIngresado.length() == usuarioEsperado.length() + 2) {
	    return true;
    }
    // Generar el correo esperado
    
    //string correoEsperado = usuarioEsperado + "00" + "@alumno.ipn.mx";

    // Comparar con el correo escrito
    /*
    if(correo == correoEsperado)
    {
        // username = usuarioEsperado;
        return true;
    }
*/
    return false;
}

bool Usuario::validarEscuela(string escuela) {
	vector<string> escuelasIPN = {"ESIME","ESIA","ESIQUIE","ESIQ","ESIT","ESCOM","UPIITA","UPIBI",
				      "UPIIG","UPIIZ","UPIIH","UPIIT","UPIIP","UPII","UPIICSA","ENCB",
				      "ESM","CICS","ESCA","ESE","EST","ESFM","ESEO","UPIIC","UPIIY",
				      "ENMH","ENBA"};
	
	string escuelaNormalizada = normalizarTexto(escuela);

	for(const string& escuelaIPN : escuelasIPN) {
		if(normalizarTexto(escuelaIPN) == escuelaNormalizada) {
		       return true;
		}
	}

	return false;	
}

string Usuario::normalizarTexto(const string& texto) {
	string resultado = "";
	
	for(char c : texto) {
		if(!isspace(c)) {
			resultado += toupper(c);
		}
	}

	return resultado;
}
