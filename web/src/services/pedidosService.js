import { request } from './api';

const extraerPedido = (data) => {
  if (data && typeof data === 'object' && 'pedido' in data) return data.pedido;
  return data?.data ?? null;
};

export const normalizePedido = (pedido) => {
  if (!pedido) return null;

  const items = Array.isArray(pedido.items)
    ? pedido.items.map((item, index) => ({
        id: item.id ?? item.productId ?? item._id ?? `${pedido.id ?? 'pedido'}-${index}`,
        name: item.name ?? item.producto ?? item.product ?? 'Producto',
        quantity: Number(item.quantity ?? item.cantidad ?? 1),
        price: Number(item.price ?? item.precio ?? item.unitPrice ?? 0),
        image: item.image ?? item.imagen ?? '',
      }))
    : [];

  const total = Number(pedido.total ?? items.reduce((sum, item) => sum + item.price * item.quantity, 0));

  return {
    id: pedido.id ?? pedido._id ?? pedido.numero ?? '#UPI-0000',
    fecha: pedido.fecha ?? pedido.createdAt ?? pedido.created_at ?? new Date().toISOString(),
    items,
    total,
    tiempoEstimado: Number(pedido.tiempoEstimado ?? pedido.estimatedTime ?? pedido.tiempo ?? 15),
    estatus: pedido.estatus ?? pedido.status ?? 'En preparación',
    usuarioId: pedido.usuarioId ?? pedido.userId ?? '',
  };
};

// Crea el pedido en la base de datos. Solo viajan el id y la cantidad de cada
// producto: el precio y el total los calcula el servidor con los datos reales.
export const createPedido = async ({ items = [], cafeteriaId } = {}) => {
  const data = await request('/api/pedidos', {
    method: 'POST',
    body: JSON.stringify({
      ...(cafeteriaId ? { cafeteriaId } : {}),
      items: items.map((item) => ({ id: item.id, quantity: item.quantity })),
    }),
  });

  return normalizePedido(extraerPedido(data));
};

export const getPedidos = async () => {
  const data = await request('/api/pedidos');
  const pedidos = Array.isArray(data?.pedidos) ? data.pedidos : [];

  return pedidos.map(normalizePedido).sort((a, b) => new Date(b.fecha) - new Date(a.fecha));
};

export const getPedidoActivo = async () => {
  const data = await request('/api/pedidos/activo');
  return normalizePedido(extraerPedido(data));
};

export const getPedidoById = async (pedidoId) => {
  if (!pedidoId) return null;

  const data = await request(`/api/pedidos/${encodeURIComponent(String(pedidoId))}`);
  return normalizePedido(extraerPedido(data));
};
