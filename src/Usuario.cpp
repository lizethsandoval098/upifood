#include "Usuario.h"

Usuario::Usuario(){ }

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra)
                : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra} {
}	
		
Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contra, string username, int anio, string escuela) : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, contra{contra}, username{username}, anio{anio}, escuela{escuela} {
}
			
Usuario::~Usuario(){ }

string Usuario::getNombre(){
	return nombre;
}

string Usuario::getTipoUsuario(){
	return tipoUsuario;
}

string Usuario::getCorreo(){
	return correo;
}

string Usuario::getContra(){
	return contra;
}

void Usuario::setNombre(string nom){
	nombre = nom;
}

void Usuario::setTipoUsuario(string tipou){
	tipoUsuario = tipou;
}

void Usuario::setCorreo(string cor){
	correo = cor;
}
void Usuario::setContra(string con){
	contra = con;
}
		
void Usuario::iniciarSesion(){
	
}
		void registro();
		bool validarIPN(string correo, int anio, string escuela);
		void asignarUsername(string correo);

// Métodos
//=====================

void Usuario::registro()
{
    cout << "==================================" << endl;
    cout << "Registro de usuario" << endl;
//Para poder asignar un USERNAME y dejar acceder al sistema al usuario se necesita validar su informacion ya que estos usuarios que tienen la posiblidad de registrarse deben ser de la comunidad del IPN por ello se hace la comprobacion por medio de un IF si su informacion proporcionada en los campos solicitados en el registro es veridica y esto lo hace por medio del metodo (VALIDAR IPN) el cual solo devuelve 1 o 0 y en caso de que sea 1 sus datos son correctos 
    if(validarIPN(correo, anio, escuela))
    {
//Si al ingresar los datos y compararlos con el correo coiniciden se genera su USERNAME en base a su correo (RECUERDOQUE ERA CON ELNOMBRE Y EL AÑO COMPARADOS CON EL CORREO??????????)
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
//Primero se verifica que la variable de correo (POSIBLEMENTE ME EQUIVOQUE Y SEA CON EL NOMBRE) este vacia para que no haya algun dato que se haya quedado al,acenado en laa variable ya que empty es una funcion de string (ESTO RECUERDO QUE LO LLEGAMOS A USAR EN OTRO SEMESTRE PARA VALIDAR UNA VARIABLE VACIA) 
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
//Una vez que ya haya validado que sus campos de correo y contraseña estuvieran vacios y que  el usuaio registrar suinformacion en este putno se supone que se iria a la base de datos a validar esos datos y permitirle el acceso al sistema
    cout << "Credenciales recibidas." << endl;
    cout << "Inicio de sesion correcto." << endl;
}

//De acuerdo a los datos que el usuario ingrese de correo, año y escuela 
bool Usuario::validarIPN(string correo,
                         int anio,
                         string escuela)
{
//SE VALIDA QUE EL CORREO NO ESTE VACIO ya que si esta vacio pues no se podria validar al usuario, se hace con EMPTY el cual indica si una variable esta vacia o no y en este caso si esta vacio retorna un false y como la funcion es bool debe retornar o true o false y si no cumple se manda un false
    if(correo.empty())
        return false;
//Se valida que ingrese un año valido ya que no puede ser un año menor o igual a 0 pues seria una incongruencia ,  (TENGO DUDA EN SI PUEDE SER CUALQUIER AÑO YA QUE ESTO ES "FICTICIO" ) por lo que el año debe ser cualquiera siempre y cuando pues no sea algo menor o igual a 0
    if(anio <= 0)
        return false;
//Lo mismo que sucede con el correo
    if(escuela.empty())
        return false;

    // Variable donde se guaradara el username que se generara al recibir los datos del usuario TENGO DUDAAA, ES QUE LA FORMA MAS FACIL PARA MII ES SEPARARLO EN NOMBRE, APELLIDO MATERNO, APELLIDO PATERNO. Para que con la posicion de los caracteres se forme el no,bre de usuario y se compare con el correo 
    string usuarioEsperado;

    usuarioEsperado += nombre[0]; //Se toma el primer caracter 
    usuarioEsperado += apellidoPaterno;//Se agrega el appelido completo ya que en los correos el apellido completo es el paterno
    usuarioEsperado += apellidoMaterno[0];//Se toma solo el primer caracter del apellido materno 
   //El año se pide en "int" sin embargo para poder crear el username se debe de tratar como string porque pues se van a unir datos de tipo String por ello se hace  el casteo de int a String con "to string" para tratar ese dato  entero como string. LA VERDAAAD LO DE substr si lo pregunte a la IA ya que no sabia como hacer que tomara de la cadena la pisicion de cierto caracter hacia adelante y la cantidad. EJEMPLO 2026 no sabia como hacer para que solo tomara 26 osea la posicion 2 mas el digito siguiente, es decir 2 digitos 
    usuarioEsperado += to_string(anio).substr(2,2);

    // Generar el correo esperado
    string correoEsperado = usuarioEsperado + "@alumno.ipn.mx"; // En teori es lo mismo que si pusieran como tal el correo ya que solo se agrega el @alumno.ipn.mx

    // Comparar con el correo escrito
    if(correo == correoEsperado)
    {
        username = usuarioEsperado;
        return true;
    }

    return false;
}
