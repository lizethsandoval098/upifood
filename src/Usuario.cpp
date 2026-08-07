#include "Usuario.h"

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
	
}
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);

void Usuario::registro()
{
    cout << "==================================" << endl;
    cout << "Registro de usuario" << endl;

    if(validarIPN(correo, anio, escuela))
    {
        asignarUsername(correo);

        cout << "Registro realizado correctamente." << endl;
        cout << "Username asignado: " << username << endl;
    }
    else
    {
        cout << "No es posible registrar el usuario." << endl;
        cout << "El correo institucional no es valido." << endl;
    }
}

void Usuario::iniciarSesion()
{
    cout << "==================================" << endl;
    cout << "Inicio de sesion" << endl;

    if(correo.empty())
    {
        cout << "Debe ingresar un correo." << endl;
        return;
    }

    if(contrasena.empty())
    {
        cout << "Debe ingresar una contraseña." << endl;
        return;
    }

    cout << "Credenciales recibidas." << endl;
    cout << "Inicio de sesion correcto." << endl;
}

//Para que ese pueda validar si el usuario es del instituto o no se requiere comparar su correo con sus datos de año escuela (TALVEZ ERA CON EL NOMBRE IGUAL)
bool Usuario::validarIPN(string correo, int anio, string escuela)
{
    if(correo.empty())
        return false;

    if(anio <= 0)
        return false;

    if(escuela.empty())
        return false;

    // Generar el username esperado
    string usuarioEsperado;

    usuarioEsperado += nombre[0];
    usuarioEsperado += apellidoPaterno;
    usuarioEsperado += apellidoMaterno[0];
    usuarioEsperado += to_string(anio).substr(2,2);

    // Generar el correo esperado
    string correoEsperado = usuarioEsperado + "@alumno.ipn.mx";

    // Comparar con el correo escrito
    if(correo == correoEsperado)
    {
        username = usuarioEsperado;
        return true;
    }

    return false;
}


