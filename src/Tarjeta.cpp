#include "Tarjeta.h"

Tarjeta::Tarjeta() {
    numeroTarjeta = 0;
    CVV = 0;
    nombrePropietario = "";
    fechaVencimiento = "";
}

// Constructor
Tarjeta::Tarjeta(int numeroTarjeta, int CVV, string nombrePropietario, string fechaVencimiento)
    : numeroTarjeta(numeroTarjeta), CVV(CVV), nombrePropietario(nombrePropietario),
	fechaVencimiento(fechaVencimiento) {
}

// Destructor
Tarjeta::~Tarjeta() {
}

// Getters
string Tarjeta::getNombrePropietario() const {
    return nombrePropietario;
}

int Tarjeta::getNumeroTarjeta() const {
    return numeroTarjeta;
}

int Tarjeta::getCVV() const {
    return CVV;
}

string Tarjeta::getFechaVencimiento() const {
    return fechaVencimiento;
}

string Tarjeta::getUsernamePropietario() const {
    return nombrePropietario;
}

// Setters
void Tarjeta::setNombrePropietario(const string& nombre) {
    nombrePropietario = nombre;
}

void Tarjeta::setNumeroTarjeta(int numero) {
    numeroTarjeta = numero;
}

void Tarjeta::setCVV(int cvv) {
	CVV = cvv;
}

void Tarjeta::setFechaVencimiento(const string& fecha) {
    fechaVencimiento = fecha;
}
