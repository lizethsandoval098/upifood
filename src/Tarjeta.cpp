#include "Tarjeta.h"

Tarjeta::Tarjeta() {
    numeroTarjeta = "";
    CVV = "";
    nombrePropietario = "";
    fechaVencimiento = "";
}

// Constructor
Tarjeta::Tarjeta(string numeroTarjeta, string CVV, string nombrePropietario, string fechaVencimiento)
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

string Tarjeta::getNumeroTarjeta() const {
    return numeroTarjeta;
}

string Tarjeta::getCVV() const {
    return CVV;
}

string Tarjeta::getFechaVencimiento() const {
    return fechaVencimiento;
}

// Setters
void Tarjeta::setNombrePropietario(const string& nombre) {
    nombrePropietario = nombre;
}

void Tarjeta::setNumeroTarjeta(const string& numero) {
    numeroTarjeta = numero;
}

void Tarjeta::setCVV(const string& cvv) {
    CVV = cvv;
}

void Tarjeta::setFechaVencimiento(const string& fecha) {
    fechaVencimiento = fecha;
}
