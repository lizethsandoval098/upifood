#include "Tarjeta.h"

// Constructor
Tarjeta::Tarjeta() {
    numeroTarjeta = 0;
    CVV = 0;
    nombrePropietario = "";
    fechaVencimiento = "";
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

// Setters (Lectura desde la consola)
void Tarjeta::setNombrePropietario() {
    cout << "Ingrese el nombre del propietario de la tarjeta: ";
    cin.ignore();
    getline(cin, nombrePropietario);
}

void Tarjeta::setNumeroTarjeta() {
    cout << "Ingrese el numero de tarjeta: ";
    cin >> numeroTarjeta;
}

void Tarjeta::setCVV() {
    cout << "Ingrese el CVV: ";
    cin >> CVV;
}

void Tarjeta::setFechaVencimiento() {
    cout << "Ingrese la fecha de vencimiento (MM/AA): ";
    cin >> fechaVencimiento;
}
