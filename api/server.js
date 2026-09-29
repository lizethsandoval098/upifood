// =====================================================================
// UPIIFOOD - API web  (puente HTTP  <->  servidor C++ por TCP)
//
//   Navegador (Vercel)  --HTTPS-->  Tailscale Funnel  -->  esta API (127.0.0.1:3000)
//                                                              |
//                                                              +--TCP--> servidor C++ (127.0.0.1:5000) --> SQLite
//
// La web NUNCA habla con el servidor C++ ni con SQLite directamente: solo con
// esta API. La API solo expone las rutas de abajo (lista blanca); no reenvia
// comandos arbitrarios, asi que nadie puede, por ejemplo, borrar productos o
// cerrar la caja desde internet.
// =====================================================================
import express from 'express';
import cors from 'cors';
import net from 'net';
import crypto from 'crypto';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// ---------------------------------------------------------------------
// Configuracion (variables de entorno o archivo api/.env)
// ---------------------------------------------------------------------
function cargarEnv() {
  const archivo = path.join(__dirname, '.env');
  if (!fs.existsSync(archivo)) return;

  for (const linea of fs.readFileSync(archivo, 'utf8').split(/\r?\n/)) {
    const m = linea.match(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*?)\s*$/);
    if (m && process.env[m[1]] === undefined) {
      process.env[m[1]] = m[2].replace(/^["']|["']$/g, '');
    }
  }
}
cargarEnv();

const HTTP_PORT = Number(process.env.PORT || 3000);
const HTTP_HOST = process.env.HOST || '127.0.0.1'; // 127.0.0.1: solo Tailscale/localhost llegan
const TCP_HOST = process.env.UPIIFOOD_TCP_HOST || '127.0.0.1';
const TCP_PORT = Number(process.env.UPIIFOOD_TCP_PORT || 5000);

// Origenes (paginas web) que pueden llamar a la API. Separados por coma.
const WEB_ORIGINS = (process.env.WEB_ORIGIN || 'http://localhost:5173,http://127.0.0.1:5173')
  .split(',')
  .map((o) => o.trim().replace(/\/$/, ''))
  .filter(Boolean);

// Secreto para firmar sesiones. Si no lo pones en el entorno se genera una vez
// y se guarda en api/.secreto (no lo subas a GitHub).
function obtenerSecreto() {
  if (process.env.UPIIFOOD_API_SECRET) return process.env.UPIIFOOD_API_SECRET;

  const archivo = path.join(__dirname, '.secreto');
  if (fs.existsSync(archivo)) return fs.readFileSync(archivo, 'utf8').trim();

  const nuevo = crypto.randomBytes(48).toString('hex');
  fs.writeFileSync(archivo, nuevo, { mode: 0o600 });
  return nuevo;
}
const SECRETO = obtenerSecreto();

const app = express();
app.set('trust proxy', 1); // detras de Tailscale Funnel / serve
app.disable('x-powered-by');

app.use(
  cors({
    origin(origin, cb) {
      // sin Origin = curl/Postman/mismo equipo
      if (!origin || WEB_ORIGINS.includes('*') || WEB_ORIGINS.includes(origin.replace(/\/$/, ''))) {
        return cb(null, true);
      }
      return cb(new Error('Origen no permitido por CORS.'));
    },
  })
);
app.use(express.json({ limit: '20kb' }));

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------
class ErrorHttp extends Error {
  constructor(status, mensaje) {
    super(mensaje);
    this.status = status;
  }
}

const enviarError = (res, status, mensaje) => res.status(status).json({ ok: false, error: mensaje });

// El protocolo del servidor C++ usa '|' para separar campos y '\n' para
// separar comandos. Si dejaramos pasar esos caracteres desde internet, alguien
// podria "colar" comandos extra. Todo dato que entra se valida aqui.
function textoSeguro(valor, campo, { max = 80, permitirVacio = false } = {}) {
  const texto = String(valor ?? '').trim();

  if (!texto && !permitirVacio) throw new ErrorHttp(400, `Falta ${campo}.`);
  if (texto.length > max) throw new ErrorHttp(400, `${campo} es demasiado largo.`);
  if (/[|\r\n\0,:]/.test(texto)) throw new ErrorHttp(400, `${campo} contiene caracteres no permitidos.`);

  return texto;
}

function idSeguro(valor, campo) {
  const id = String(valor ?? '').trim();
  if (!/^[A-Za-z0-9_-]{1,40}$/.test(id)) throw new ErrorHttp(400, `Identificador invalido: ${campo}.`);
  return id;
}

// ---------------------------------------------------------------------
// Cliente TCP hacia el servidor C++ (una conexion por comando)
// Devuelve un arreglo de lineas. Sabe leer respuestas de varias lineas.
// ---------------------------------------------------------------------
function lineasEsperadas(comando, lineas) {
  if (lineas.length === 0) return 1;

  const cabecera = lineas[0].split('|');
  if (cabecera[0] !== 'OK') return 1; // error: una sola linea

  const n = Number(cabecera[1]);
  const tipo = comando.split('|')[0];

  if (tipo === 'LISTAR_CAFETERIAS' || tipo === 'INVENTARIO') {
    return 1 + (Number.isFinite(n) ? n : 0);
  }

  if (tipo === 'PEDIDOS_CLIENTE' || tipo === 'PEDIDO_CONSULTAR') {
    let indice = 1;

    for (let i = 0; i < n; i++) {
      if (indice >= lineas.length) return indice + 1; // aun falta la cabecera de este pedido
      const items = Number(lineas[indice].split('|')[6]) || 0;
      indice += 1 + items;
    }

    return indice;
  }

  return 1;
}

function enviarComando(comando, { timeoutMs = 5000 } = {}) {
  return new Promise((resolve, reject) => {
    const socket = new net.Socket();
    let buffer = '';
    let terminado = false;

    const finalizar = (error, resultado) => {
      if (terminado) return;
      terminado = true;
      socket.destroy();
      if (error) reject(error);
      else resolve(resultado);
    };

    socket.setTimeout(timeoutMs);

    socket.connect(TCP_PORT, TCP_HOST, () => {
      socket.write(`${comando}\n`);
    });

    socket.on('data', (data) => {
      buffer += data.toString('utf8');

      const partes = buffer.split('\n');
      partes.pop(); // lo ultimo (posible linea incompleta o vacia)
      const lineas = partes.map((l) => l.replace(/\r$/, ''));

      if (lineas.length > 0 && lineas.length >= lineasEsperadas(comando, lineas)) {
        finalizar(null, lineas);
      }
    });

    socket.on('timeout', () => finalizar(new Error('Tiempo de espera agotado con el servidor C++.')));
    socket.on('error', (e) => finalizar(e));
    socket.on('close', () => {
      if (!terminado) {
        // el servidor cerro: entregar lo que llego si hay algo
        const lineas = buffer.split('\n').filter((l) => l.length > 0).map((l) => l.replace(/\r$/, ''));
        if (lineas.length > 0) finalizar(null, lineas);
        else finalizar(new Error('El servidor C++ cerro la conexion sin responder.'));
      }
    });
  });
}

// Ejecuta un comando y lanza ErrorHttp si el servidor respondio ERR|...
async function comandoCpp(comando, statusError = 400) {
  let lineas;

  try {
    lineas = await enviarComando(comando);
  } catch (error) {
    console.error('[API -> C++] fallo de conexion:', error.message);
    throw new ErrorHttp(503, 'No se pudo conectar con el servidor de la cafeteria.');
  }

  const cabecera = lineas[0].split('|');

  if (cabecera[0] !== 'OK') {
    throw new ErrorHttp(statusError, cabecera.slice(1).join('|') || 'El servidor rechazo la operacion.');
  }

  return lineas;
}

// ---------------------------------------------------------------------
// Contrasenas y sesiones
// ---------------------------------------------------------------------
// La base de datos guarda lo que reciba en "contrasena". Para no dejar la
// contrasena real en SQLite, la API guarda un hash (scrypt) y el servidor C++
// simplemente compara ese hash al iniciar sesion.
function hashContrasena(username, password) {
  return crypto.scryptSync(password, `upifood:${username}`, 32).toString('hex');
}

const base64url = (buf) => Buffer.from(buf).toString('base64url');

function firmar(payload) {
  const cuerpo = base64url(JSON.stringify(payload));
  const firma = crypto.createHmac('sha256', SECRETO).update(cuerpo).digest('base64url');
  return `${cuerpo}.${firma}`;
}

function verificarToken(token) {
  const [cuerpo, firma] = String(token || '').split('.');
  if (!cuerpo || !firma) return null;

  const esperada = crypto.createHmac('sha256', SECRETO).update(cuerpo).digest('base64url');
  const a = Buffer.from(firma);
  const b = Buffer.from(esperada);

  if (a.length !== b.length || !crypto.timingSafeEqual(a, b)) return null;

  try {
    const payload = JSON.parse(Buffer.from(cuerpo, 'base64url').toString('utf8'));
    return payload.exp > Date.now() ? payload : null;
  } catch {
    return null;
  }
}

const DURACION_SESION_MS = 7 * 24 * 60 * 60 * 1000;
const crearSesion = (usuario, invitado = false) => firmar({ u: usuario, inv: invitado, exp: Date.now() + DURACION_SESION_MS });

function exigirSesion(req, res, next) {
  const encabezado = req.headers.authorization || '';
  const payload = verificarToken(encabezado.startsWith('Bearer ') ? encabezado.slice(7) : '');

  if (!payload) return enviarError(res, 401, 'Sesion invalida o expirada. Inicia sesion de nuevo.');

  req.sesion = { username: payload.u, invitado: Boolean(payload.inv) };
  next();
}

// ---------------------------------------------------------------------
// Limite de peticiones (en memoria) para que nadie sature la API publica
// ---------------------------------------------------------------------
function limitar({ ventanaMs, maximo }) {
  const contadores = new Map();

  setInterval(() => {
    const ahora = Date.now();
    for (const [clave, dato] of contadores) if (dato.reinicio < ahora) contadores.delete(clave);
  }, ventanaMs).unref();

  return (req, res, next) => {
    const ahora = Date.now();
    const dato = contadores.get(req.ip) || { cuenta: 0, reinicio: ahora + ventanaMs };

    if (dato.reinicio < ahora) {
      dato.cuenta = 0;
      dato.reinicio = ahora + ventanaMs;
    }

    dato.cuenta += 1;
    contadores.set(req.ip, dato);

    if (dato.cuenta > maximo) return enviarError(res, 429, 'Demasiadas peticiones. Intenta en un momento.');
    next();
  };
}

app.use('/api', limitar({ ventanaMs: 60_000, maximo: 300 }));
const limiteAuth = limitar({ ventanaMs: 10 * 60_000, maximo: 30 });

// Envuelve handlers async para no repetir try/catch
const ruta = (fn) => async (req, res) => {
  try {
    await fn(req, res);
  } catch (error) {
    if (error instanceof ErrorHttp) return enviarError(res, error.status, error.message);
    console.error('Error inesperado:', error);
    return enviarError(res, 500, 'Error interno del servidor.');
  }
};

// ---------------------------------------------------------------------
// Conversion de datos C++ -> formato que ya usa la web
// ---------------------------------------------------------------------
const ESTATUS_WEB = {
  Pendiente: 'En preparación',
  Preparando: 'En preparación',
  Listo: 'Listo para recolección',
  Entregado: 'Entregado',
  Cancelado: 'Cancelado',
};

function fechaIso(fechaDb) {
  const fecha = new Date(String(fechaDb).replace(' ', 'T')); // hora local del servidor
  return Number.isNaN(fecha.getTime()) ? new Date().toISOString() : fecha.toISOString();
}

// lineas = ["OK|M", "folio|estado|total|fecha|idCafeteria|username|N", "id|nombre|cant|precio", ...]
function leerPedidos(lineas) {
  const n = Number(lineas[0].split('|')[1]) || 0;
  const pedidos = [];
  let i = 1;

  for (let k = 0; k < n; k++) {
    const [folio, estado, total, fecha, cafeteriaId, usuarioId, cantItems] = lineas[i].split('|');
    const items = [];

    for (let j = 1; j <= Number(cantItems); j++) {
      const [id, nombre, cantidad, precio] = lineas[i + j].split('|');
      items.push({ id, name: nombre, quantity: Number(cantidad), price: Number(precio), image: '' });
    }

    i += 1 + Number(cantItems);

    pedidos.push({
      id: folio,
      fecha: fechaIso(fecha),
      items,
      total: Number(total),
      tiempoEstimado: estado === 'Pendiente' || estado === 'Preparando' ? 20 : 0,
      estatus: ESTATUS_WEB[estado] || estado,
      estadoInterno: estado,
      cafeteriaId,
      usuarioId,
    });
  }

  return pedidos;
}

const ESTADOS_FINALES = ['Entregado', 'Cancelado'];

// ---------------------------------------------------------------------
// Rutas
// ---------------------------------------------------------------------
app.get(
  '/api/health',
  ruta(async (req, res) => {
    await comandoCpp('LISTAR_CAFETERIAS', 502);
    res.json({ ok: true, servidorCpp: true });
  })
);

// ---- Menu (sale de la base de datos, la misma que edita la cafeteria) ----
app.get(
  '/api/cafeterias',
  ruta(async (req, res) => {
    const lineas = await comandoCpp('LISTAR_CAFETERIAS');
    const cafeterias = lineas.slice(1).map((l) => {
      const [id, nombre] = l.split('|');
      return { id, nombre };
    });

    res.json({ ok: true, cafeterias });
  })
);

app.get(
  '/api/cafeterias/:id/productos',
  ruta(async (req, res) => {
    const id = idSeguro(req.params.id, 'La cafeteria');
    const lineas = await comandoCpp(`INVENTARIO|${id}`);

    const productos = lineas.slice(1).map((l) => {
      const [pid, nombre, stock, precio] = l.split('|');
      return { id: pid, nombre, stock: Number(stock), precio: Number(precio) };
    });

    res.json({ ok: true, cafeteriaId: id, productos });
  })
);

// ---- Cuentas ----
const ESCUELAS_IPN = ['upiita', 'esime', 'esca', 'esfm', 'encb', 'upiih', 'cecyt'];

app.post(
  '/api/auth/registro',
  limiteAuth,
  ruta(async (req, res) => {
    const nombreCompleto = textoSeguro(req.body.nombreCompleto, 'El nombre', { max: 80 });
    const correo = textoSeguro(req.body.correo, 'El correo', { max: 80 }).toLowerCase();
    const password = String(req.body.password || '');
    const escuela = String(req.body.escuela || '').toLowerCase();

    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(correo)) throw new ErrorHttp(400, 'El correo no es valido.');
    if (password.length < 6 || password.length > 100) throw new ErrorHttp(400, 'La contraseña debe tener entre 6 y 100 caracteres.');

    const username = correo; // el correo es el usuario en la base de datos
    const tipoCliente = escuela === 'upiita' ? 'UPIITA' : ESCUELAS_IPN.includes(escuela) ? 'IPN' : 'EXTERNO';

    const comando = ['REGISTRO_CLIENTE', username, nombreCompleto, correo, hashContrasena(username, password), '-', '-', tipoCliente].join('|');

    try {
      await comandoCpp(comando);
    } catch (error) {
      if (error instanceof ErrorHttp && error.status === 400) {
        throw new ErrorHttp(409, 'Ya existe una cuenta con ese correo.');
      }
      throw error;
    }

    res.status(201).json({
      ok: true,
      token: crearSesion(username),
      usuario: { username, nombreCompleto, correo, tipoCliente },
    });
  })
);

app.post(
  '/api/auth/login',
  limiteAuth,
  ruta(async (req, res) => {
    const username = textoSeguro(req.body.correo, 'El correo', { max: 80 }).toLowerCase();
    const password = String(req.body.password || '');
    if (!password) throw new ErrorHttp(400, 'Falta la contraseña.');

    let lineas;
    try {
      lineas = await comandoCpp(`LOGIN_CLIENTE|${username}|${hashContrasena(username, password)}`);
    } catch (error) {
      if (error instanceof ErrorHttp && error.status === 400) {
        throw new ErrorHttp(401, 'El correo o la contraseña son incorrectos.');
      }
      throw error;
    }

    // OK|nombre|correo|apellidoP|apellidoM|tipoCliente|username
    const [, nombre, correo, , , tipoCliente] = lineas[0].split('|');

    res.json({
      ok: true,
      token: crearSesion(username),
      usuario: { username, nombreCompleto: nombre, correo, tipoCliente },
    });
  })
);

// Los invitados de la web piden bajo una cuenta compartida "invitado_web".
const USUARIO_INVITADO = 'invitado_web';
let invitadoListo = false;

async function asegurarInvitado() {
  if (invitadoListo) return;

  const clave = crypto.randomBytes(24).toString('hex'); // nadie inicia sesion con esta cuenta
  const comando = ['REGISTRO_CLIENTE', USUARIO_INVITADO, 'Invitado Web', `${USUARIO_INVITADO}@upifood.local`, clave, '-', '-', 'INVITADO'].join('|');

  try {
    await comandoCpp(comando);
  } catch (error) {
    // si ya existia, el servidor responde ERR y esta bien
    if (!(error instanceof ErrorHttp && error.status === 400)) throw error;
  }

  invitadoListo = true;
}

app.post(
  '/api/auth/invitado',
  limiteAuth,
  ruta(async (req, res) => {
    await asegurarInvitado();
    res.json({ ok: true, token: crearSesion(USUARIO_INVITADO, true), usuario: { username: USUARIO_INVITADO, invitado: true } });
  })
);

// ---- Pedidos ----
async function cafeteriaPorDefecto() {
  const lineas = await comandoCpp('LISTAR_CAFETERIAS');
  if (lineas.length < 2) throw new ErrorHttp(503, 'No hay cafeterias registradas.');
  return lineas[1].split('|')[0];
}

app.post(
  '/api/pedidos',
  exigirSesion,
  limitar({ ventanaMs: 60_000, maximo: 20 }),
  ruta(async (req, res) => {
    const { items } = req.body;
    if (!Array.isArray(items) || items.length === 0) throw new ErrorHttp(400, 'El pedido esta vacio.');
    if (items.length > 30) throw new ErrorHttp(400, 'El pedido tiene demasiados productos.');

    // Se junta por producto (si el mismo id llega dos veces se suman)
    const cantidades = new Map();
    for (const item of items) {
      const id = idSeguro(item.id, 'El producto');
      const cantidad = Number(item.quantity);
      if (!Number.isInteger(cantidad) || cantidad < 1 || cantidad > 20) throw new ErrorHttp(400, 'Cantidad invalida.');
      cantidades.set(id, (cantidades.get(id) || 0) + cantidad);
    }

    if (req.sesion.invitado) await asegurarInvitado();

    const cafeteriaId = req.body.cafeteriaId ? idSeguro(req.body.cafeteriaId, 'La cafeteria') : await cafeteriaPorDefecto();
    const lista = [...cantidades].map(([id, cantidad]) => `${id}:${cantidad}`).join(',');

    // El total NO viene de la web: lo calcula el servidor C++ con los precios de la BD.
    const comando = `PEDIDO_CREAR|${cafeteriaId}|${req.sesion.username}|${lista}`;
    console.log('[WEB -> C++]', comando);

    const respuesta = await comandoCpp(comando);
    console.log('[C++ -> API]', respuesta[0]);

    const folio = respuesta[0].split('|')[1];
    const detalle = await comandoCpp(`PEDIDO_CONSULTAR|${folio}`);

    res.status(201).json({ ok: true, pedido: leerPedidos(detalle)[0] });
  })
);

app.get(
  '/api/pedidos',
  exigirSesion,
  ruta(async (req, res) => {
    // Los invitados comparten cuenta: no se les muestra historial.
    if (req.sesion.invitado) return res.json({ ok: true, pedidos: [] });

    const lineas = await comandoCpp(`PEDIDOS_CLIENTE|${req.sesion.username}`);
    res.json({ ok: true, pedidos: leerPedidos(lineas) });
  })
);

app.get(
  '/api/pedidos/activo',
  exigirSesion,
  ruta(async (req, res) => {
    if (req.sesion.invitado) return res.json({ ok: true, pedido: null });

    const lineas = await comandoCpp(`PEDIDOS_CLIENTE|${req.sesion.username}`);
    const activo = leerPedidos(lineas).find((p) => !ESTADOS_FINALES.includes(p.estadoInterno)) || null;
    res.json({ ok: true, pedido: activo });
  })
);

app.get(
  '/api/pedidos/:id',
  exigirSesion,
  ruta(async (req, res) => {
    const folio = idSeguro(req.params.id, 'El pedido');
    const lineas = await comandoCpp(`PEDIDO_CONSULTAR|${folio}`, 404);
    const pedido = leerPedidos(lineas)[0];

    // Solo el dueño del pedido puede verlo
    if (pedido.usuarioId !== req.sesion.username) throw new ErrorHttp(404, 'Pedido no encontrado.');

    res.json({ ok: true, pedido });
  })
);

app.use('/api', (req, res) => enviarError(res, 404, 'Ruta no encontrada.'));

app.use((error, req, res, next) => {
  if (error?.message === 'Origen no permitido por CORS.') return enviarError(res, 403, error.message);
  if (error?.type === 'entity.parse.failed') return enviarError(res, 400, 'JSON invalido.');
  console.error('Error:', error);
  return enviarError(res, 500, 'Error interno del servidor.');
});

app.listen(HTTP_PORT, HTTP_HOST, () => {
  console.log(`API UPIIFOOD escuchando en http://${HTTP_HOST}:${HTTP_PORT}`);
  console.log(`Servidor C++: ${TCP_HOST}:${TCP_PORT}`);
  console.log(`Origenes web permitidos: ${WEB_ORIGINS.join(', ')}`);
});
