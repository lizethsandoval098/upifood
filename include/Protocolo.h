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
// Formato de los mensajes (pipe-delimited), definido por nosotras:
//   Peticion  : COMANDO|campo1|campo2|...
//   Respuesta : OK|campo1|campo2|...
//               ERR|mensaje de error
//
// Para respuestas con listas (ej. "dame todos los productos"), se manda
// primero "OK|N" (N = numero de registros) y luego N mensajes mas, uno por
// registro, cada uno "campo1|campo2|...".

bool enviarMensaje(int socket, const string& mensaje);
bool recibirMensaje(int socket, string& mensajeRecibido);

// Junta enviarMensaje + recibirMensaje: manda una peticion y regresa la
// respuesta completa (para el lado cliente/cafeteria/administrador).
string enviarComando(int socket, const string& comando);

// Separa "a|b|c" en {"a","b","c"}. Util para leer tanto peticiones como
// respuestas del lado que las recibe.
vector<string> separarCampos(const string& mensaje, char separador = '|');

#endif
