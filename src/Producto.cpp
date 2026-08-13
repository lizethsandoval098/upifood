#include "Producto.h"
#include "BaseDatos.h"

Producto::Producto(){
}

Producto::Producto(string nombreProducto, string idProducto, int stock, float precio)
    : nombreProducto{nombreProducto}, idProducto{idProducto},
      stock{stock}, precio{precio} {
}

Producto::~Producto(){
}

string Producto::getNombreProducto() const{
    return nombreProducto;
}

string Producto::getIdProducto() const{
    return idProducto;
}

int Producto::getStock() const{
    return stock;
}

float Producto::getPrecio() const{
    return precio;
}

void Producto::setNombreProducto(const string& nombrep){
    nombreProducto = nombrep;
}

void Producto::setIdProducto(const string& idp){
    idProducto = idp;
}

void Producto::setStock(int sto){
    stock = sto;
}

void Producto::setPrecio(float pre){
    precio = pre;
}
