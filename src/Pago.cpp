#include "Pago.h"

// Constructor
Pago::Pago() {
    monto = 0.0f;
    aprobado = false;
    folioPedido = "";
}

// Destructor
Pago::~Pago() {
}

// Getters
const Tarjeta& Pago::getTarjeta() const {
    return tarjeta;
}

float Pago::getMonto() const {
    return monto;
}

bool Pago::getAprobado() const {
    return aprobado;
}

string Pago::getFolioPedido() const {
    return folioPedido;
}

// Setters 
void Pago::asignarTarjeta(const Tarjeta& t) {
    tarjeta = t;
}

void Pago::setMonto(float m) {
    monto = m;
}

void Pago::setAprobado(bool a) {
    aprobado = a;
}

void Pago::setFolioPedido(const string& folio) {
    folioPedido = folio;
}

string Pago::getFechaPago() const { return fechaPago; }
string Pago::getReferencia() const { return referencia; }
string Pago::getMotivo() const { return motivo; }
string Pago::getUsernameCliente() const { return usernameCliente; }
string Pago::getIdCafeteria() const { return idCafeteria; }

void Pago::setFechaPago(const string& f) { fechaPago = f; }
void Pago::setReferencia(const string& r) { referencia = r; }
void Pago::setMotivo(const string& m) { motivo = m; }
void Pago::setUsernameCliente(const string& u) { usernameCliente = u; }
void Pago::setIdCafeteria(const string& c) { idCafeteria = c; }
