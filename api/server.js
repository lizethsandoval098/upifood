import express from 'express';
import cors from 'cors';
import net from 'net';

const app = express();

const HTTP_PORT = Number(process.env.PORT || 3000);

const TCP_HOST = process.env.UPIIFOOD_TCP_HOST || '127.0.0.1';
const TCP_PORT = Number(process.env.UPIIFOOD_TCP_PORT || 5000);

const WEB_ORIGIN = process.env.WEB_ORIGIN || '*';

app.use(cors({
  origin: WEB_ORIGIN === '*' ? true : WEB_ORIGIN
}));

app.use(express.json());

function enviarComando(comando) {
  return new Promise((resolve, reject) => {
    const socket = new net.Socket();

    let respuesta = '';
    let terminado = false;

    const finalizar = (error, resultado) => {
      if (terminado) return;

      terminado = true;
      socket.destroy();

      if (error) {
        reject(error);
      } else {
        resolve(resultado);
      }
    };

    socket.setTimeout(5000);

    socket.connect(TCP_PORT, TCP_HOST, () => {
      socket.write(`${comando}\n`);
    });

    socket.on('data', (data) => {
      respuesta += data.toString();

      const posicion = respuesta.indexOf('\n');

      if (posicion !== -1) {
        const linea = respuesta.slice(0, posicion);
        finalizar(null, linea);
      }
    });

    socket.on('timeout', () => {
      finalizar(new Error('Tiempo de espera agotado con el servidor C++.'));
    });

    socket.on('error', (error) => {
      finalizar(error);
    });

    socket.on('close', () => {
      if (!terminado) {
        finalizar(
          new Error('El servidor C++ cerró la conexión sin responder.')
        );
      }
    });
  });
}

function separarCampos(mensaje) {
  return mensaje.split('|');
}

function respuestaError(res, mensaje, status = 400) {
  return res.status(status).json({
    ok: false,
    error: mensaje
  });
}

app.get('/api/health', async (req, res) => {
  try {
    const respuesta = await enviarComando('LISTAR_CAFETERIAS');

    if (!respuesta.startsWith('OK|')) {
      return respuestaError(
        res,
        'El servidor C++ respondió con error.',
        502
      );
    }

    res.json({
      ok: true,
      servidorCpp: true
    });

  } catch (error) {
    console.error(error);

    respuestaError(
      res,
      'No se pudo conectar con el servidor C++.',
      503
    );
  }
});

app.post('/api/pedidos', async (req, res) => {
  try {
    const {
      usuarioId,
      cafeteriaId,
      items
    } = req.body;

    if (!usuarioId) {
      return respuestaError(res, 'Falta el usuario.');
    }

    if (!cafeteriaId) {
      return respuestaError(res, 'Falta la cafetería.');
    }

    if (!Array.isArray(items) || items.length === 0) {
      return respuestaError(res, 'El pedido está vacío.');
    }

    const productos = items.map((item) => {
      const id = String(item.id || '');
      const cantidad = Number(item.quantity);

      if (!id || !Number.isInteger(cantidad) || cantidad <= 0) {
        throw new Error('Producto o cantidad inválida.');
      }

      return `${id}:${cantidad}`;
    });

    const listaItems = productos.join(',');

    const comando =
      `PEDIDO_CREAR|${cafeteriaId}|${usuarioId}|${listaItems}`;

    console.log('[WEB → C++]', comando);

    const respuesta = await enviarComando(comando);

    console.log('[C++ → API]', respuesta);

    const campos = separarCampos(respuesta);

    if (campos[0] !== 'OK') {
      return respuestaError(
        res,
        campos.slice(1).join('|') || 'No se pudo crear el pedido.',
        400
      );
    }

    res.status(201).json({
      ok: true,
      pedido: {
        id: campos[1],
        total: Number(campos[2]),
        cafeteriaId: campos[3],
        usuarioId
      }
    });

  } catch (error) {
    console.error('Error creando pedido:', error);

    return respuestaError(
      res,
      error.message || 'Error interno del servidor.',
      500
    );
  }
});

app.listen(HTTP_PORT, '127.0.0.1', () => {
  console.log(`API UPIIFOOD escuchando en http://127.0.0.1:${HTTP_PORT}`);
  console.log(`Servidor C++: ${TCP_HOST}:${TCP_PORT}`);
});
