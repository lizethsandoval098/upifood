import { useEffect } from 'react';
import { getPedidoActivo, getPedidoById } from '../services/pedidosService';

export function usePedidoRealtime({ enabled = false, usuarioId = 'usuario-demo', pedidoId = null, onPedidoUpdate = () => {} }) {
  useEffect(() => {
    if (!enabled) return undefined;

    let ignore = false;

    const refreshPedido = async () => {
      try {
        const nextPedido = pedidoId ? await getPedidoById(pedidoId) : await getPedidoActivo(usuarioId);

        if (!ignore && nextPedido) {
          onPedidoUpdate(nextPedido);
        }
      } catch {
        // Se ignora el error de polling para no romper la experiencia del usuario.
      }
    };

    refreshPedido();
    const intervalId = window.setInterval(refreshPedido, 5000);

    return () => {
      ignore = true;
      window.clearInterval(intervalId);
    };
  }, [enabled, pedidoId, usuarioId, onPedidoUpdate]);
}
