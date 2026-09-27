#include "Usuario.h"
#include <functional>

Usuario::Usuario() { }

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contrasenaPlano)
    : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo} {
    setContrasena(contrasenaPlano);
}

Usuario::Usuario(string tipoUsuario, string nombre, string correo, string contrasenaPlano, string username)
    : tipoUsuario{tipoUsuario}, nombre{nombre}, correo{correo}, username{username} {
    setContrasena(contrasenaPlano);
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

string Usuario::getContrasenaHash() const {
    return contrasenaHash;
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

void Usuario::setContrasena(const string& contrasenaPlano) {
    contrasenaHash = hashContrasena(contrasenaPlano);
}

void Usuario::setContrasenaHash(const string& hashYaCalculado) {
    contrasenaHash = hashYaCalculado;
}

bool Usuario::verificarContrasena(const string& intento) const {
    return contrasenaHash == hashContrasena(intento);
}

string Usuario::hashContrasena(const string& contrasenaPlano) {
    // Hash simple con una "sal" fija, solo para no guardar texto plano.
    // OJO: para produccion real usar bcrypt/Argon2, esto NO es seguro contra
    // ataques serios, pero cumple con no dejar la contrasena visible en la BD.
    static const string sal = "upifood_2026_sal";
    std::hash<string> hasher;
    size_t h = hasher(sal + contrasenaPlano);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%zx", h);
    return string(buffer);
}
