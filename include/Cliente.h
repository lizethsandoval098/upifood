#ifndef CLIENTE_H
#define CLIENTE_H

#include <string>
#include <vector>
#include <iostream>

#include "Usuario.h"
#include "Tarjeta.h"
#include "Pedido.h"

using namespace std;

// ---- Estructuras sencillas para lo que el servidor le manda al cliente ----
struct InfoCafeteria {
	string id;
	string nombre;
};

struct ResumenPedidoCliente {
	string folio;
	string estado;
	string fecha;
	string idCafeteria;
	float total = 0.0f;
	bool pagado = false;
};

struct InfoTarjeta {
	string numero;
	string vencimiento;
	string enmascarada;   // "**** **** **** 1234"
};

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

		// ---- datos que se piden al servidor para la ventana grafica ----
		string ultimoError;
		vector<InfoCafeteria> cafeterias;
		vector<Producto> menu;                        // productos de la cafeteria elegida
		vector<pair<Producto, int>> carrito;          // producto + cantidad
		string idCafeteriaCarrito;                    // el carrito es de UNA sola cafeteria
		vector<ResumenPedidoCliente> pedidosCliente;  // historial / estado (via servidor)
		string folioDetalle;                          // pedido cuyo detalle esta cargado
		vector<pair<Producto, int>> detallePedido;    // productos de ese pedido (via servidor)
		vector<InfoTarjeta> tarjetasRed;              // tarjetas registradas (via servidor)

		// "OK|N" y luego N lineas (mismo patron que Administrador::pedirLista)
		bool pedirLista(const string& comando, vector<string>& lineas);

	public:
		Cliente(); // cliente invitado por defecto
		Cliente(string nombre, string correo, string contrasena, string username,
		        string apellidoPaterno, string apellidoMaterno, string tipoCliente);

		~Cliente();

		void setIpServidor(const string& ip);

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

		// ---- Registro e inicio de sesion (flujo de consola, via el servidor) ----
		// Hace todas las preguntas por consola, valida el correo institucional
		// LOCALMENTE (son puras reglas), y manda el registro al servidor por el
		// socket. Requiere haber llamado conectar() antes.
		bool registrarNuevoCliente();

		// Pide username/contrasena por consola y los manda al servidor.
		// Si el login es correcto, llena *this con los datos y regresa true.
		// Requiere haber llamado conectar() antes.
		bool iniciarSesionCliente();

		// =================================================================
		//  API para la VENTANA (SFML). Ninguna usa cin/cout: todas regresan
		//  true/false y dejan el motivo en getUltimoError(). Todas requieren
		//  haber llamado conectar() antes. NUNCA abren la base de datos: todo
		//  lo piden al servidor.
		// =================================================================
		const string& getUltimoError() const;

		// Login sin consola. Si sale bien, llena *this con los datos.
		bool iniciarSesion(const string& usuario, const string& clave);

		// Registro sin consola: valida (correo IPN, escuela, anio, contrasena)
		// y manda REGISTRO_CLIENTE. Regresa el username asignado en usernameOut.
		bool registrarse(const string& correo, const string& nombre, const string& apellidoP,
		                 const string& apellidoM, const string& anio, const string& escuela,
		                 const string& contrasena, string& usernameOut);

		bool cargarCafeterias();
		bool cargarMenu(const string& idCafeteria);
		const vector<InfoCafeteria>& getCafeterias() const;
		const vector<Producto>& getMenu() const;
		string nombreCafeteria(const string& idCafeteria) const;

		// Carrito (local, hasta que se manda con crearPedidoDesdeCarrito)
		const vector<pair<Producto, int>>& getCarrito() const;
		const string& getIdCafeteriaCarrito() const;
		int cantidadEnCarrito(const string& idProducto) const;
		int unidadesEnCarrito() const;
		float totalCarrito() const;
		bool agregarAlCarrito(const string& idCafeteria, const Producto& producto);
		void quitarUnoDelCarrito(const string& idProducto);
		void quitarDelCarrito(const string& idProducto);
		void vaciarCarrito();

		// Manda PEDIDO_CREAR. Si sale bien, vacia el carrito y regresa el folio/total.
		bool crearPedidoDesdeCarrito(string& folioOut, float& totalOut);

		bool cargarPedidosCliente();
		const vector<ResumenPedidoCliente>& getPedidosCliente() const;

		// Detalle (productos) de UN pedido del historial: PEDIDO_DETALLE.
		bool cargarDetallePedido(const string& folio, const string& idCafeteria);
		const vector<pair<Producto, int>>& getDetallePedido() const;
		const string& getFolioDetalle() const;

		bool cargarTarjetasRed();
		const vector<InfoTarjeta>& getTarjetasRed() const;
		bool agregarTarjetaRed(const string& numero, const string& cvv, const string& titular,
		                       const string& vencimiento);

		// PAGAR_PEDIDO. referenciaOut = referencia del pago si fue aprobado.
		bool pagarPedidoRed(const string& folio, const string& numeroTarjeta, const string& cvv,
		                    string& referenciaOut);

		// ---- Validaciones IPN ----
		// El correo debe ser institucional (@alumno.ipn.mx), el anio de ingreso
		// razonable, y la escuela debe existir dentro del catalogo de escuelas del IPN.
		static bool validarIPN(const string& correo, int anio, const string& escuela);
		static bool validarEscuela(const string& escuela);

		// El correo se arma con: inicial del nombre + apellido paterno + inicial del
		// apellido materno + 4 digitos, y los 2 primeros digitos son los ultimos 2
		// del anio de ingreso (2021 -> 21). Ej: Juan Perez Lopez, 2021 ->
		// jperezl21XX@alumno.ipn.mx (XX aleatorios).
		// Regresa "" si esta bien; si no, el mensaje de que esta mal.
		static string validarCorreoConNombre(const string& correo, const string& nombre,
		                                     const string& apellidoP, const string& apellidoM, int anio);

		// Minusculas, sin acentos/n con tilde, sin espacios ni signos: "Pérez Núñez" -> "pereznunez"
		static string normalizarParaCorreo(const string& texto);
		static string normalizarTexto(const string& texto);

		// Username automatico = lo que esta antes de la @ en el correo.
		static string asignarUsername(const string& correo);
};

#endif
