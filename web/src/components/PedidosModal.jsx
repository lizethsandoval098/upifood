const statusColors = {
  'En preparación': '#c77a2a',
  'Listo para recolección': '#1d7a52',
  Entregado: '#4d5d74',
  Cancelado: '#8d4638',
};

const formatDate = (isoDate) => {
  if (!isoDate) return 'Sin fecha';

  const date = new Date(isoDate);
  if (Number.isNaN(date.getTime())) return isoDate;

  return new Intl.DateTimeFormat('es-MX', {
    day: '2-digit',
    month: 'short',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
  }).format(date);
};

const getStatusBadgeStyle = (estatus) => ({
  background: `${statusColors[estatus] || '#6b4a35'}22`,
  color: statusColors[estatus] || '#6b4a35',
  border: `1px solid ${statusColors[estatus] || '#6b4a35'}55`,
});

export default function PedidosModal({ open, onClose, activePedido, historialPedidos }) {
  if (!open) return null;

  const pedidos = [...(historialPedidos || [])];
  if (activePedido && !pedidos.some((pedido) => pedido.id === activePedido.id)) {
    pedidos.unshift(activePedido);
  }

  const pedidosFinalizados = pedidos.filter((pedido) =>
    ['Entregado', 'Cancelado'].includes(pedido?.estatus)
  );
  const pedidosActivos = pedidos.filter((pedido) =>
    !['Entregado', 'Cancelado'].includes(pedido?.estatus)
  );

  return (
    <div
      style={{
        position: 'fixed',
        inset: 0,
        background: 'rgba(44, 29, 18, 0.48)',
        backdropFilter: 'blur(4px)',
        zIndex: 2000,
        display: 'grid',
        placeItems: 'center',
        padding: '20px',
      }}
      onClick={onClose}
    >
      <div
        style={{
          width: 'min(1120px, 100%)',
          maxHeight: '85vh',
          overflowY: 'auto',
          background: '#fffdfb',
          borderRadius: '28px',
          border: '1px solid rgba(93, 67, 49, 0.12)',
          boxShadow: '0 20px 46px rgba(60, 42, 33, 0.25)',
        }}
        onClick={(event) => event.stopPropagation()}
      >
        <div
          style={{
            padding: '24px 28px 18px',
            borderBottom: '1px solid rgba(93, 67, 49, 0.1)',
            background: 'linear-gradient(180deg, #fdf7f0 0%, #f7eee4 100%)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            gap: '12px',
          }}
        >
          <div>
            <p style={{ margin: '0 0 8px', color: '#8a5a2b', fontSize: '0.8rem', fontWeight: 700, letterSpacing: '2px', textTransform: 'uppercase' }}>
              Mis pedidos
            </p>
            <h2 style={{ margin: 0, color: '#3c2a21', fontFamily: 'Playfair Display, serif', fontSize: 'clamp(2rem, 3vw, 2.7rem)' }}>
              Seguimiento de orden
            </h2>
          </div>
          <button
            type="button"
            onClick={onClose}
            style={{
              border: 'none',
              width: '42px',
              height: '42px',
              borderRadius: '999px',
              background: '#fff7ef',
              color: '#3c2a21',
              fontSize: '1.3rem',
              fontWeight: 800,
              cursor: 'pointer',
            }}
            aria-label="Cerrar mis pedidos"
          >
            ×
          </button>
        </div>

        <div style={{ display: 'grid', gridTemplateColumns: '1.3fr 0.9fr', gap: '22px', padding: '22px 28px 28px' }}>
          <section style={{ display: 'grid', gap: '18px' }}>
            <div style={{ background: '#fff9f4', border: '1px solid #f0d7b9', borderRadius: '20px', padding: '18px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: '12px', marginBottom: '14px' }}>
                <h3 style={{ margin: 0, color: '#8a5a2b', fontSize: '0.82rem', fontWeight: 700, letterSpacing: '1.5px', textTransform: 'uppercase' }}>
                  Pedido en curso
                </h3>
                <span style={{ color: '#7a5b45', fontSize: '0.9rem', fontWeight: 700 }}>{pedidosActivos.length}</span>
              </div>

              {!pedidosActivos.length ? (
                <div style={{ padding: '8px 0', color: '#6d4f3b' }}>Todavía no tienes pedidos activos.</div>
              ) : (
                <div style={{ display: 'grid', gap: '12px', maxHeight: '430px', overflowY: 'auto', paddingRight: '6px' }}>
                  {pedidosActivos.map((pedido) => (
                    <article key={pedido.id} style={{ background: '#fffdfb', border: '1px solid #f0d7b9', borderRadius: '16px', padding: '14px' }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', gap: '12px', marginBottom: '12px' }}>
                        <h4 style={{ margin: 0, color: '#3c2a21', fontSize: '1.2rem' }}>{pedido.id}</h4>
                        <span style={{ ...getStatusBadgeStyle(pedido.estatus), borderRadius: '999px', padding: '7px 10px', fontWeight: 700, fontSize: '0.78rem' }}>
                          {pedido.estatus}
                        </span>
                      </div>

                      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, minmax(0,1fr))', gap: '8px', marginBottom: '12px' }}>
                        <div style={{ background: '#fff3e7', borderRadius: '12px', padding: '10px', border: '1px solid #e7c9a3' }}>
                          <div style={{ fontSize: '0.68rem', color: '#7a5b45', textTransform: 'uppercase', letterSpacing: '0.6px' }}>Fecha</div>
                          <strong style={{ display: 'block', marginTop: '5px', color: '#3c2a21', fontSize: '0.86rem' }}>{formatDate(pedido.fecha)}</strong>
                        </div>
                        <div style={{ background: '#fff3e7', borderRadius: '12px', padding: '10px', border: '1px solid #e7c9a3' }}>
                          <div style={{ fontSize: '0.68rem', color: '#7a5b45', textTransform: 'uppercase', letterSpacing: '0.6px' }}>Tiempo estimado</div>
                          <strong style={{ display: 'block', marginTop: '5px', color: '#3c2a21', fontSize: '0.86rem' }}>{pedido.tiempoEstimado} min</strong>
                        </div>
                        <div style={{ background: '#fff3e7', borderRadius: '12px', padding: '10px', border: '1px solid #e7c9a3' }}>
                          <div style={{ fontSize: '0.68rem', color: '#7a5b45', textTransform: 'uppercase', letterSpacing: '0.6px' }}>Total</div>
                          <strong style={{ display: 'block', marginTop: '5px', color: '#3c2a21', fontSize: '0.86rem' }}>${pedido.total}</strong>
                        </div>
                      </div>

                      <div style={{ display: 'grid', gap: '8px' }}>
                        {(pedido.items || []).map((item, index) => (
                          <div key={`${pedido.id}-${item.id ?? index}`} style={{ display: 'flex', justifyContent: 'space-between', gap: '12px', padding: '9px 10px', borderRadius: '10px', background: '#fffaf5', border: '1px solid #efdcc2' }}>
                            <div>
                              <strong style={{ color: '#402d1f' }}>{item.name}</strong>
                              <div style={{ color: '#755a41', fontSize: '0.88rem' }}>Cantidad: {item.quantity}</div>
                            </div>
                            <span style={{ color: '#8b5728', fontWeight: 800 }}>${item.quantity * item.price}</span>
                          </div>
                        ))}
                      </div>
                    </article>
                  ))}
                </div>
              )}
            </div>

            <div style={{ background: '#fff9f4', border: '1px solid #f0d7b9', borderRadius: '20px', padding: '18px' }}>
              <h3 style={{ margin: '0 0 14px', color: '#3c2a21', fontSize: '1.35rem' }}>Historial de pedidos</h3>

              {!pedidosFinalizados.length ? (
                <div style={{ color: '#6d4f3b' }}>Aún no hay pedidos entregados.</div>
              ) : (
                <div style={{ display: 'grid', gap: '12px' }}>
                  {pedidosFinalizados.map((pedido) => (
                    <div key={pedido.id} style={{ border: '1px solid #f0d7b9', background: '#fffaf5', borderRadius: '16px', padding: '14px' }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: '12px', marginBottom: '8px' }}>
                        <strong style={{ color: '#3c2a21' }}>{pedido.id}</strong>
                        <span style={{ ...getStatusBadgeStyle(pedido.estatus), borderRadius: '999px', padding: '6px 10px', fontWeight: 700, fontSize: '0.76rem' }}>
                          {pedido.estatus}
                        </span>
                      </div>
                      <div style={{ color: '#725740', fontSize: '0.9rem', marginBottom: '10px' }}>{formatDate(pedido.fecha)}</div>
                      <div style={{ display: 'grid', gap: '6px' }}>
                        {(pedido.items || []).map((item, index) => (
                          <div key={`${pedido.id}-${item.id ?? index}`} style={{ display: 'flex', justifyContent: 'space-between', gap: '12px', color: '#4a3020' }}>
                            <span>{item.name} x{item.quantity}</span>
                            <span>${item.quantity * item.price}</span>
                          </div>
                        ))}
                      </div>
                      <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: '10px', paddingTop: '10px', borderTop: '1px solid rgba(93, 67, 49, 0.08)', color: '#3c2a21' }}>
                        <span>Total</span>
                        <strong>${pedido.total}</strong>
                      </div>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </section>

          <aside style={{ background: '#f8efe7', border: '1px solid #ead5b6', borderRadius: '20px', padding: '18px', alignSelf: 'start' }}>
            <h3 style={{ margin: '0 0 14px', color: '#3c2a21', fontSize: '1.25rem' }}>Resumen general</h3>
            <div style={{ display: 'grid', gap: '10px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', color: '#5c4636' }}>
                <span>Pedidos activos</span>
                <strong>{pedidosActivos.length}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', color: '#5c4636' }}>
                <span>Pedidos entregados</span>
                <strong>{pedidosFinalizados.length}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', color: '#5c4636' }}>
                <span>Valor total</span>
                <strong>${pedidos.reduce((sum, pedido) => sum + Number(pedido?.total || 0), 0)}</strong>
              </div>
            </div>
          </aside>
        </div>
      </div>
    </div>
  );
}
