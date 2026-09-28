const API_BASE_URL = (import.meta.env.VITE_API_URL || 'http://localhost:3000').replace(/\/$/, '');

const getHeaders = (extraHeaders = {}) => {
  const token = localStorage.getItem('upifood_token');

  return {
    'Content-Type': 'application/json',
    ...(token ? { Authorization: `Bearer ${token}` } : {}),
    ...extraHeaders,
  };
};

async function request(path, options = {}) {
  const response = await fetch(`${API_BASE_URL}${path}`, {
    ...options,
    headers: getHeaders(options.headers || {}),
  });

  const contentType = response.headers.get('content-type') || '';
  const responseBody = contentType.includes('application/json') ? await response.json() : await response.text();

  if (!response.ok) {
    const message =
      typeof responseBody === 'string'
        ? responseBody
        : responseBody?.message || responseBody?.error || `La petición falló con estado ${response.status}`;

    throw new Error(message);
  }

  return responseBody;
}

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

  const total = Number(
    pedido.total ??
      items.reduce((sum, item) => sum + item.price * item.quantity, 0) ??
      0
  );

  return {
    id: pedido.id ?? pedido._id ?? pedido.numero ?? '#UPI-0000',
    fecha: pedido.fecha ?? pedido.createdAt ?? pedido.created_at ?? new Date().toISOString(),
    items,
    total,
    tiempoEstimado: Number(pedido.tiempoEstimado ?? pedido.estimatedTime ?? pedido.tiempo ?? 15),
    estatus: pedido.estatus ?? pedido.status ?? 'En preparación',
    usuarioId: pedido.usuarioId ?? pedido.userId ?? 'usuario-demo',
  };
};

export const createPedido = async ({ usuarioId = 'usuario-demo', items = [], total = 0, metodoPago = 'tarjeta' }) => {
  const payload = {
    usuarioId,
    metodoPago,
    total,
    items: items.map((item) => ({
      id: item.id,
      name: item.name,
      quantity: item.quantity,
      price: item.price,
      image: item.image || '',
    })),
  };

  const data = await request('/api/pedidos', {
    method: 'POST',
    body: JSON.stringify(payload),
  });

  const pedido = data?.pedido ?? data?.data ?? data;
  return normalizePedido(pedido);
};

export const getPedidos = async (usuarioId = 'usuario-demo') => {
  const data = await request(`/api/pedidos?usuarioId=${encodeURIComponent(usuarioId)}`);
  const pedidos = Array.isArray(data)
    ? data
    : Array.isArray(data?.pedidos)
      ? data.pedidos
      : Array.isArray(data?.data)
        ? data.data
        : [];

  return pedidos.map(normalizePedido).sort((first, second) => new Date(second.fecha) - new Date(first.fecha));
};

export const getPedidoActivo = async (usuarioId = 'usuario-demo') => {
  const data = await request(`/api/pedidos/activo?usuarioId=${encodeURIComponent(usuarioId)}`);
  const pedido = data?.pedido ?? data?.data ?? data;
  return normalizePedido(pedido);
};

export const getPedidoById = async (pedidoId) => {
  if (!pedidoId) return null;

  const data = await request(`/api/pedidos/${encodeURIComponent(String(pedidoId))}`);
  const pedido = data?.pedido ?? data?.data ?? data;
  return normalizePedido(pedido);
};
