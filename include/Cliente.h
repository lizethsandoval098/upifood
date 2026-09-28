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
		bool estaConectado() const;

		// Cliente INVITADO: no tiene cuenta; se le asigna un username temporal
		// "inv_<pid>_<hora>" y el servidor lo da de alta solo al hacer su primer pedido.
		void entrarComoInvitado(const string& nombreCompleto);

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

		// Todo esto va POR EL SOCKET al servidor (el cliente ya no abre la BD).
		void cargarTarjetasGuardadas();   // TARJETAS|user
		void cargarHistorial();           // HISTORIAL|user
		void cargarPedidoActual();        // el ultimo pedido no entregado/cancelado del historial

		void verHistorialP();
		void hacerPedido();               // elige cafeteria + productos -> CREAR_PEDIDO
		void pagar();                     // PAGAR|folio|tarjeta
		void recogerPedido();             // Listo -> Entregado (CAMBIAR_ESTADO)
		void verEstadoPedido();           // ESTADO_PEDIDO|folio

		// ---- Registro e inicio de sesion (flujo de consola, via el servidor) ----
		// Hace todas las preguntas por consola, valida el correo institucional
		// LOCALMENTE (son puras reglas), y manda el registro al servidor por el
		// socket. Requiere haber llamado conectar() antes.
		bool registrarNuevoCliente();

		// Pide username/contrasena por consola y los manda al servidor.
		// Si el login es correcto, llena *this con los datos y regresa true.
		// Requiere haber llamado conectar() antes.
		bool iniciarSesionCliente();

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
