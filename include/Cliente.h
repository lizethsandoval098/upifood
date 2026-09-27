#ifndef CLIENTE_H
#define CLIENTE_H

#include <string>
#include <vector>
#include <iostream>

#include "Usuario.h"
#include "Tarjeta.h"
#include "Pedido.h"

using namespace std;

// Cliente hereda de Usuario: nombre, correo, contrasenaHash, username y
// tipoUsuario ("Cliente") ya vienen de la clase base. Aqui solo se agrega
// lo que es especifico de un cliente.
class Cliente : public Usuario {

	private:
		string apellidoPaterno;
		string apellidoMaterno;
		string tipoCliente; // "UPIITA", "IPN", "EXTERNO" o "INVITADO"

		// conexion al servidor
		int socketCliente;
		string ipServidor;
		int puerto;

		vector<Tarjeta> tarjetasGuardadas;
		vector<Pedido> historialPedidos;
		Pedido pedidoActual;

	public:
		Cliente(); // cliente invitado por defecto
		Cliente(string nombre, string correo, string contrasena, string username,
		        string apellidoPaterno, string apellidoMaterno, string tipoCliente);

		~Cliente();

		bool conectar();

		string getApellidoPaterno() const;
		string getApellidoMaterno() const;
		string getTipoCliente() const;
		vector<Tarjeta> getTarjetasGuardadas() const;
		vector<Pedido> getHistorialPedidos() const;
		Pedido& getPedidoActual();

		void setApellidoPaterno(const string& ap);
		void setApellidoMaterno(const string& am);
		void setTipoCliente(const string& tc);
		void setTarjetasGuardadas(const vector<Tarjeta>& tarjetas);
		void setHistorialPedidos(const vector<Pedido>& historial);
		void setPedidoActual(const Pedido& pedido);

		void cargarTarjetasGuardadas();
		void cargarPedidoActual();

		void verHistorialP();
		void hacerPedido();
		void pagar();
		void recogerPedido();
		void verEstadoPedido();

		// ---- Registro e inicio de sesion (flujo de consola) ----
		// Hace todas las preguntas por consola, valida el correo institucional,
		// genera el username y guarda el cliente en la base de datos.
		// Regresa true si el registro se completo con exito.
		static bool registrarNuevoCliente();

		// Pide username/contrasena por consola y valida contra la base de datos.
		// Si el login es correcto, llena "out" con los datos del cliente y regresa true.
		static bool iniciarSesionCliente(Cliente& out);

		// ---- Validaciones IPN ----
		// El correo debe ser institucional (@alumno.ipn.mx), el anio de ingreso
		// razonable, y la escuela debe existir dentro del catalogo de escuelas del IPN.
		static bool validarIPN(const string& correo, int anio, const string& escuela);
		static bool validarEscuela(const string& escuela);
		static string normalizarTexto(const string& texto);

		// Username automatico = lo que esta antes de la @ en el correo.
		static string asignarUsername(const string& correo);
};

#endif

