#include "Pedido.h"
#include "BaseDatos.h"

Pedido::Pedido(){
}

Pedido::Pedido(string folio, string usernameCliente, string idCafeteria,
               float total, vector<pair<Producto, int>> listaProductos)
    : folio{folio}, usernameCliente{usernameCliente},
      idCafeteria{idCafeteria}, total{total},
      listaProductos{listaProductos} {
}

Pedido::~Pedido(){
}

string Pedido::getFolio() const{
    return folio;
}

string Pedido::getFecha() const{
    return fecha;
}

string Pedido::getEstado() const{
    return estado;
}

string Pedido::getUsernameCliente() const{
    return usernameCliente;
}

string Pedido::getIdCafeteria() const{
    return idCafeteria;
}

float Pedido::getTotal() const{
    return total;
}

CodigoQR Pedido::getQr() const{
    return qr;
}

vector<pair<Producto, int>> Pedido::getListaProductos() const{
    return listaProductos;
}

void Pedido::setFolio(const string& f){
    folio = f;
}

void Pedido::setFecha(const string& f){
    fecha = f;
}

void Pedido::setEstado(const string& e){
    estado = e;
}

void Pedido::setUsernameCliente(const string& uc){
    usernameCliente = uc;
}

void Pedido::setIdCafeteria(const string& ic){
    idCafeteria = ic;
}

void Pedido::setTotal(float t){
    total = t;
}

void Pedido::setQr(const string& url, bool valido){
    qr = CodigoQR(url, valido);
} 


void Pedido::agregarProducto(const Producto& producto) {
	
}

void Pedido::eliminarProducto(const string& idp);

void Pedido::mostrarCarrito();

void Pedido::vaciarCarrito();



void Pedido::generarTotal();

void Pedido::generarTicket();

void Pedido::cambiarEstado();

void Pedido::visualizar();



