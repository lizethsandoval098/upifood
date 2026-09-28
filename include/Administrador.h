#ifndef ADMINISTRADOR_H
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>

#include "Usuario.h"
#include "Cafeteria.h"

using namespace std;

class Administrador : public Usuario {
private:
    vector<Usuario> listaUsuarios;
    vector<Cafeteria> listaCafeterias;

    int socketAdmin;
    string ipServidor;
    int puerto;

public:
    Administrador();
    Administrador(string nombre, string correo, string contrasena, string username);

    ~Administrador();

    bool conectar();

    const vector<Usuario>& getListaUsuarios() const;
    const vector<Cafeteria>& getListaCafeterias() const;

    void cargarUsuarios();
    void verUsuarios();
    void verCafeterias();
    void verUsuariosEnLinea();

    bool iniciarSesionAdmin();
};

#endif
