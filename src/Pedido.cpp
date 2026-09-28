#include "Pedido.h"
#include <iomanip>

using namespace std;

Pedido::Pedido() : total(0.0f) {
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

void Pedido::setListaProductos(const vector<pair<Producto, int>>& lista){
    listaProductos = lista;
}

void Pedido::agregarProducto(const Producto& producto, int cantidad) {
	if (cantidad <= 0) return;

	for (auto& par : listaProductos) {
		if (par.first.getIdProducto() == producto.getIdProducto()) {
			par.second += cantidad;
			return;
		}
	}

	listaProductos.push_back(make_pair(producto, cantidad));
}

bool Pedido::eliminarProducto(const string& idp) {
	for (auto it = listaProductos.begin(); it != listaProductos.end(); ++it) {
		if (it->first.getIdProducto() == idp) {
			listaProductos.erase(it);
			return true;
		}
	}

	return false;
}

void Pedido::mostrarCarrito() {
	cout << "\n========================================================" << endl;
    	cout << "                  CARRITO DE COMPRAS                    " << endl;
    	cout << "========================================================" << endl;
    	cout << "  Cliente : " << usernameCliente << endl;
    	cout << "  Cafeteria : " << idCafeteria << endl;
    	cout << "--------------------------------------------------------" << endl;

    	if (listaProductos.empty()) {
        	cout << "   El carrito esta vacio." << endl;
        	cout << "========================================================" << endl;
        	return;
    	}

    	cout << left
             << setw(8)  << "ID"
             << setw(22) << "PRODUCTO"
             << setw(8)  << "CANT."
             << setw(10) << "TOTAL" << endl;

    	cout << "--------------------------------------------------------" << endl;

	for (const auto& par : listaProductos) {
        	const Producto& prod = par.first;
        	int cantidad = par.second;
        	double subtotal = prod.getPrecio() * cantidad;

        	cout << left
             	     << setw(8)  << prod.getIdProducto()
             	     << setw(22) << prod.getNombreProducto()
             	     << setw(8)  << cantidad
             	     << "$" << fixed << setprecision(2) << subtotal
             	     << endl;
    	}

   	cout << "--------------------------------------------------------" << endl;
    	cout << "          TOTAL PARCIAL: $" << fixed << setprecision(2) << total << endl;
    	cout << "========================================================" << endl;
}

void Pedido::vaciarCarrito() {
	listaProductos.clear();
	total = 0;
}

void Pedido::generarTotal() {
	total = 0.0;

	for(const auto& par : listaProductos) {
       		total += (par.first.getPrecio() * par.second);
	}
}

void Pedido::generarTicket() {
	cout << "\n========================================================" << endl;
    	cout << "                  TICKET DE COMPRA                      " << endl;
    	cout << "========================================================" << endl;
    	cout << "  Folio: #" << folio << endl;
	cout << "  Fecha: " << fecha << endl;
	cout << "  Cliente: " << usernameCliente << endl;
	cout << "  Cafeteria : " << idCafeteria << endl;
    	cout << "--------------------------------------------------------" << endl;

    	cout << left
             << setw(8)  << "ID"
             << setw(22) << "PRODUCTO"
             << setw(8)  << "CANT."
             << setw(10) << "TOTAL" << endl;

    	cout << "--------------------------------------------------------" << endl;

	for (const auto& par : listaProductos) {
		const Producto& prod = par.first;
		int cantidad = par.second;
		double subtotal = prod.getPrecio() * cantidad;
		cout << left << setw(20) << prod.getNombreProducto()
		     << " x" << cantidad << " $"
		     << fixed << setprecision(2) << subtotal << endl;
	}

	cout << "--------------------------------------------------------" << endl;
	cout << "                           TOTAL A PAGAR: $" << fixed << setprecision(2) << total << endl;
        cout << "--------------------------------------------------------" << endl;
        cout << "           Folio para recoger: #" << folio << endl;
	cout << "========================================================\n" << endl;
}

void Pedido::cambiarEstado(const string& nuevo) {
	estado = nuevo;
}

void Pedido::visualizar() {
	cout << "[Folio #" << folio << "]" << endl;
	cout << left << setw(36) << ("| Cafeteria : " +  idCafeteria) << "|" << endl;
	cout << left << setw(36) << ("| No. de productos : " + to_string(listaProductos.size())) << "|" << endl;
        cout << left << setw(36) << ("| Total : $" + to_string(total)) << "|" << endl;
	cout << left << setw(36) << ("| Estado : " + estado) << "|" << endl;
}

bool Pedido::haExpirado(long segundosSimuladosTranscurridos) const {
	const long UNA_HORA_SIMULADA = 3600;
	return segundosSimuladosTranscurridos >= UNA_HORA_SIMULADA;
}
