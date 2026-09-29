#include "Tarjeta.h"
#include <ctime>
#include <cstdio>

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

// "1234567812345678" -> "**** **** **** 5678"
string Tarjeta::enmascarada() const {
    if (numeroTarjeta.size() < 4) {
        return "****";
    }

    return "**** **** **** " + numeroTarjeta.substr(numeroTarjeta.size() - 4);
}

bool Tarjeta::estaVencida() const {
    // Formato esperado: MM/AA
    if (fechaVencimiento.size() != 5 || fechaVencimiento[2] != '/') {
        return true;
    }

    for (int i : {0, 1, 3, 4}) {
        if (fechaVencimiento[i] < '0' || fechaVencimiento[i] > '9') {
            return true;
        }
    }

    int mes = (fechaVencimiento[0] - '0') * 10 + (fechaVencimiento[1] - '0');
    int anio = 2000 + (fechaVencimiento[3] - '0') * 10 + (fechaVencimiento[4] - '0');

    if (mes < 1 || mes > 12) {
        return true;
    }

    time_t ahora = time(nullptr);
    tm* local = localtime(&ahora);
    int anioActual = local->tm_year + 1900;
    int mesActual = local->tm_mon + 1;

    return anio < anioActual || (anio == anioActual && mes < mesActual);
}

static bool soloDigitosExactos(const string& texto, size_t largo) {
    if (texto.size() != largo) {
        return false;
    }

    for (char c : texto) {
        if (c < '0' || c > '9') {
            return false;
        }
    }

    return true;
}

bool Tarjeta::numeroFormatoValido(const string& numero) {
    return soloDigitosExactos(numero, 16);
}

bool Tarjeta::cvvFormatoValido(const string& cvv) {
    return soloDigitosExactos(cvv, 3);
}

bool Tarjeta::fechaFormatoValida(const string& fecha) {
    if (fecha.size() != 5 || fecha[2] != '/') {
        return false;
    }

    if (!soloDigitosExactos(fecha.substr(0, 2), 2) || !soloDigitosExactos(fecha.substr(3, 2), 2)) {
        return false;
    }

    int mes = (fecha[0] - '0') * 10 + (fecha[1] - '0');
    return mes >= 1 && mes <= 12;
}

bool Tarjeta::luhnValido(const string& numero) {
    if (numero.size() < 12 || numero.size() > 19) {
        return false;
    }

    int suma = 0;
    bool duplicar = false;

    for (int i = static_cast<int>(numero.size()) - 1; i >= 0; i--) {
        if (numero[i] < '0' || numero[i] > '9') {
            return false;
        }

        int digito = numero[i] - '0';

        if (duplicar) {
            digito *= 2;
            if (digito > 9) digito -= 9;
        }

        suma += digito;
        duplicar = !duplicar;
    }

    return suma % 10 == 0;
}

Tarjeta Tarjeta::generarSimulada(const string& titular, mt19937& generador) {
    uniform_int_distribution<int> distDigito(0, 9);

    // 15 digitos: prefijo 9999 + 11 aleatorios; el ultimo es el verificador.
    string base = "9999";
    for (int i = 0; i < 11; i++) {
        base += static_cast<char>('0' + distDigito(generador));
    }

    int suma = 0;
    bool duplicar = true; // el digito mas a la derecha de "base" se duplica
    for (int i = static_cast<int>(base.size()) - 1; i >= 0; i--) {
        int digito = base[i] - '0';

        if (duplicar) {
            digito *= 2;
            if (digito > 9) digito -= 9;
        }

        suma += digito;
        duplicar = !duplicar;
    }

    base += static_cast<char>('0' + (10 - suma % 10) % 10);

    string cvv;
    for (int i = 0; i < 3; i++) {
        cvv += static_cast<char>('0' + distDigito(generador));
    }

    uniform_int_distribution<int> distMeses(24, 60);
    time_t ahora = time(nullptr);
    tm* local = localtime(&ahora);
    int mesesTotales = local->tm_year * 12 + local->tm_mon + distMeses(generador);
    int anio = (mesesTotales / 12 + 1900) % 100;
    int mes = mesesTotales % 12 + 1;

    char vence[8];
    snprintf(vence, sizeof(vence), "%02d/%02d", mes, anio);

    return Tarjeta(base, cvv, titular, vence);
}
