#include "Usuario.h"
#include <functional>

Usuario::Usuario() { }

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contrasena)
    : nombre{nombre}, correo{correo}, tipoUsuario{tipoUsuario} {
    setContrasena(contrasena);
}

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contrasena, string username)
    : nombre{nombre}, correo{correo}, tipoUsuario{tipoUsuario}, username{username} {
    setContrasena(contrasena);
}

Usuario::~Usuario() { }

string Usuario::getNombre() const {
    return nombre;
}

string Usuario::getTipoUsuario() const {
    return tipoUsuario;
}

string Usuario::getCorreo() const {
    return correo;
}

string Usuario::getContrasena() const {
    return contrasena;
}

string Usuario::getUsername() const {
    return username;
}

void Usuario::setNombre(const string& n) {
    nombre = n;
}

void Usuario::setTipoUsuario(const string& tu) {
    tipoUsuario = tu;
}

void Usuario::setCorreo(const string& c) {
    correo = c;
}

void Usuario::setUsername(const string& u) {
    username = u;
}

void Usuario::setContrasena(const string& c) {
    contrasena = c;
}

bool Usuario::verificarContrasena(const string& intento) const {
    return contrasena == intento;
}


