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
  });

