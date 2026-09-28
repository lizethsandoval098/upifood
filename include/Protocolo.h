#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>

using namespace std;

// Funciones de bajo nivel para mandar/recibir UN mensaje completo por un
// socket TCP ya conectado. Un mensaje = una linea de texto terminada en '\n'.
// TCP no garantiza que un solo send() llegue completo en un solo recv(), por
// eso no basta con hacer send()/recv() directo: hay que juntar bytes hasta
// encontrar el '\n'.
//
// Formato de los mensajes (pipe-delimited):
//   Peticion  : COMANDO|campo1|campo2|...
//   Respuesta : OK|campo1|campo2|...
//               ERR|mensaje de error
//
// Para respuestas con listas (ej. "dame todos los productos"), se manda
// primero "OK|N" (N = numero de registros) y luego N mensajes mas, uno por
// registro, cada uno "campo1|campo2|...".
//
// ---------------------------------------------------------------------
// COMANDOS DEL SERVIDOR (resumen)
//   LOGIN_CLIENTE|user|pass            REGISTRO_CLIENTE|user|nom|correo|pass|ap|am|tipo
//   LOGIN_CAFETERIA|user|pass          LOGIN_ADMIN|user|pass
//   LISTAR_USUARIOS                    LISTAR_CAFETERIAS      LISTAR_USUARIOS_EN_LINEA
//   INVENTARIO|idCaf                   RESTOCK|idProd|cant    VENTA_CAJA|idProd|cant
//   CREAR_PEDIDO|user|idCaf|id:cant;id:cant     -> OK|folio|total
//   PAGAR|folio|numTarjeta                       -> OK|APROBADO|folio|monto  / ERR
//   ESTADO_PEDIDO|folio                          -> OK|folio|estado|total|pagado|idCaf|user
//   CAMBIAR_ESTADO|folio|nuevoEstado             -> OK|folio|estado
//   LISTAR_PEDIDOS|idCaf      HISTORIAL|user     TARJETAS|user
//   EVENTOS|desdeId           ESTADISTICAS
// ---------------------------------------------------------------------

bool enviarMensaje(int socket, const string& mensaje);
bool recibirMensaje(int socket, string& mensajeRecibido);

// Junta enviarMensaje + recibirMensaje: manda una peticion y regresa la
// respuesta completa (para el lado cliente/cafeteria/administrador).
string enviarComando(int socket, const string& comando);

// Lee 'cantidad' mensajes seguidos (las N lineas que siguen a "OK|N").
vector<string> leerLineas(int socket, int cantidad);

// Separa "a|b|c" en {"a","b","c"}. Conserva campos vacios ("a||c" -> 3 campos).
vector<string> separarCampos(const string& mensaje, char separador = '|');

// Direccion del servidor. Por defecto la IP de Tailscale original; se puede
// cambiar SIN recompilar con las variables de entorno UPIIFOOD_IP y
// UPIIFOOD_PORT (ej. UPIIFOOD_IP=127.0.0.1 para probar en una sola compu).
string obtenerIpServidor();
int obtenerPuertoServidor();

// Crea un socket TCP y lo conecta. Regresa el descriptor o -1 si falla.
// Deja un timeout de recepcion para que un servidor caido no congele al cliente.
int conectarAlServidor(const string& ip, int puerto, int timeoutSegundos = 15);

#endif
