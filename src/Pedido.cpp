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


void Pedido::agregarProducto(const Producto& producto, int cantidad) {
	if (cantidad <=0) return;

	for (auto& par : listaProductos) {
		if(par.first.getId() == producto.getId()) {
			par.second += cantidad;
			return;
		}
	}

	listaProductos.push_back(make_pair(producto, cantidad));	
}

void Pedido::eliminarProducto(const string& idp) {
	for (auto it = listaProductos.begin(); it != listaProductos.end(); ++it) {
		if (it -> first.getId() == idp) {
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
    	cout << "  Cafetería : " << idCafeteria << endl;
    	cout << "--------------------------------------------------------" << endl;

    	if (listaProductos.empty()) {
        	cout << "   El carrito está vacío." << endl;
        	cout << "========================================================" << endl;
        	return;
    	}

    	cout << left
             << setw(8)  << "   ID"
             << setw(22) << "PRODUCTO"
             << setw(8)  << "CANT."
             << setw(10) << "TOTAL" << endl;
             
    	cout << "--------------------------------------------------------" << endl;

	for (const auto& par : listaProductos) {
        	const Producto& prod = par.first;
        	int cantidad = par.second;
        	double subtotal = prod.getPrecio() * cantidad;

        	cout << left
             	     << setw(8)  << "   " << prod.getId()
             	     << setw(22) << prod.getNombre()
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
	cout << "  Cafetería : " << idCafeteria << endl;
    	cout << "--------------------------------------------------------" << endl;

    	cout << left
             << setw(8)  << "   ID"
             << setw(22) << "PRODUCTO"
             << setw(8)  << "CANT."
             << setw(10) << "TOTAL" << endl;
             
    	cout << "--------------------------------------------------------" << endl;

	for (const auto& par : listaProductos) { 
			const Producto& prod = par.first; 
			int cantidad = par.second; 
			double subtotal = prod.getPrecio() * cantidad; 
			cout << left << setw(20) << prod.getNombre()
			     << " x" << cantidad << " $" 
			     << fixed << setprecision(2) << subtotal << endl;
	        } 
	} 
	
	cout << "--------------------------------------------------------" << endl; 
	cout << "                           TOTAL A PAGAR: $" << total << endl; 
        cout << "--------------------------------------------------------" << endl;
        cout << "           ¡Escanea tu QR en el mostrador!              " << endl; 
	cout << "========================================================\n" << endl; 
}  

void Pedido::cambiarEstado(const string& nuevo) {
	estado = nuevo;
}

void Pedido::visualizar() {
	cout << "[Folio #" << folio << setw(26) << " " << " ]" << endl;
	cout << left << set(36) << ("| Cafeteria : " +  idCafeteria) << "|" << endl;
	cout << left << set(36) << ("| No. de  productos : " + to_string(listaProductos.size())) << "|" << endl;
        cout << left << set(36) << ("| Total : $" + total) << "|" << endl;
	cout << left << set(36) << ("| Estado : " + estado) << "|" << endl;	
}



