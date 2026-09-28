#ifndef ADMINISTRADOR_H
#define ADMINISTRADOR_H

#include <string>
#include <iostream>
#include <vector>
#include <mutex>

#include "Usuario.h"
#include "Cafeteria.h"
#include "Pedido.h"

using namespace std;

class Administrador : public Usuario {
private:
    vector<Usuario> listaUsuarios;
    vector<Cafeteria> listaCafeterias;
    vector<Pedido> listaPedidos;
    mutable mutex mutexPedidos;

    int socketAdmin;
    string ipServidor;
    int puerto;
    mutex mutexSocket;

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
    void cargarPedidos();
    vector<Pedido> getListaPedidos() const;
    void verPedidos() const;

    bool iniciarSesionAdmin();
};

#endif
