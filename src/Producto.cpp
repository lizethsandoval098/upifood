#include "Producto.h"

Producto::Producto(){
}

Producto::Producto(string nombreProducto, string idProducto, int cantidad, float precio)
    : nombreProducto{nombreProducto}, idProducto{idProducto},
      cantidad{cantidad}, precio{precio} {
}

Producto::~Producto(){
}

string Producto::getNombreProducto() const{
    return nombreProducto;
}

string Producto::getIdProducto() const{
    return idProducto;
}

int Producto::getCantidad() const{
    return cantidad;
}

float Producto::getPrecio() const{
    return precio;
}

void Producto::setNombreProducto(const string& nombre){
    nombreProducto = nombre;
}

void Producto::setIdProducto(const string& id){
    idProducto = id;
}

void Producto::setCantidad(int cant){
    cantidad = cant;
}

void Producto::setPrecio(float prec){
    precio = prec;
}
