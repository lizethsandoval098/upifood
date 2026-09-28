import React, { useEffect, useState } from 'react';
import Navbar from './components/Navbar';
import Hero from './components/Hero';
import Menu from './components/Menu';
import OrderSection from './components/OrderSection';
import PedidosModal from './components/PedidosModal';

const DEMO_USER_ID = 'usuario-demo';

function App() {
  const [cartItems, setCartItems] = useState([]);
  const [isCartOpen, setIsCartOpen] = useState(false);
  const [isReceiptOpen, setIsReceiptOpen] = useState(false);
  const [isPaymentConfirmed, setIsPaymentConfirmed] = useState(false);
  const [receiptData, setReceiptData] = useState(null);
  const [pedidoActual, setPedidoActual] = useState(null);
  const [historialPedidos, setHistorialPedidos] = useState([]);
  const [isOrdersModalOpen, setIsOrdersModalOpen] = useState(false);
  const [isSubmittingOrder, setIsSubmittingOrder] = useState(false);
  const [pedidoError, setPedidoError] = useState('');
  const [isLoggedIn, setIsLoggedIn] = useState(() => {
    try {
      return localStorage.getItem('cafeteria_session') === 'true';
    } catch {
      return false;
    }
  });

  const cartCount = cartItems.reduce((total, item) => total + item.quantity, 0);
  const totalToPay = cartItems.reduce((total, item) => total + item.price * item.quantity, 0);

  const handleAddToCart = (item, quantity) => {
    setCartItems((currentItems) => {
      const existingItem = currentItems.find((currentItem) => currentItem.id === item.id);

      if (existingItem) {
        return currentItems.map((currentItem) =>
          currentItem.id === item.id
            ? { ...currentItem, quantity: currentItem.quantity + quantity }
            : currentItem
        );
      }

      return [...currentItems, { ...item, quantity }];
    });
  };

  const handleRemoveItem = (itemId) => {
    setCartItems((currentItems) => currentItems.filter((item) => item.id !== itemId));
  };

  const handleDecreaseItem = (itemId) => {
    setCartItems((currentItems) =>
      currentItems
        .map((item) =>
          item.id === itemId
            ? { ...item, quantity: item.quantity - 1 }
            : item
        )
        .filter((item) => item.quantity > 0)
    );
  };

  const handleIncreaseItem = (itemId) => {
    setCartItems((currentItems) =>
      currentItems.map((item) =>
        item.id === itemId ? { ...item, quantity: item.quantity + 1 } : item
      )
    );
  };

  const handleProceedToPayment = (paymentInfo) => {
    setReceiptData({
      ...paymentInfo,
      last4: (paymentInfo.cardNumber || '').replace(/\D/g, '').slice(-4),
    });
    setIsCartOpen(false);
    setIsPaymentConfirmed(false);
    setIsReceiptOpen(true);
  };

  const handleCloseReceipt = () => {
    setIsReceiptOpen(false);
    setPedidoError('');
  };

  const handleConfirmPayment = () => {
    if (!cartItems.length || isSubmittingOrder) return;

    const now = new Date();
    const createdPedido = {
      id: `#UPI-${String(now.getTime()).slice(-4)}`,
      fecha: now.toISOString(),
      items: cartItems.map((item) => ({
        id: item.id,
        name: item.name,
        quantity: item.quantity,
        price: item.price,
        image: item.image || '',
      })),
      total: totalToPay,
      tiempoEstimado: 20,
      estatus: 'En preparación',
    };

    setPedidoActual(createdPedido);
    setHistorialPedidos((currentPedidos) => [createdPedido, ...currentPedidos]);
    setCartItems([]);
    setIsReceiptOpen(false);
    setIsPaymentConfirmed(true);
    setIsOrdersModalOpen(false);
    setPedidoError('');
    setIsSubmittingOrder(false);
  };

  const handleLogin = () => {
    setIsLoggedIn(true);
  };

  const handleLogout = () => {
    setIsLoggedIn(false);
    setCartItems([]);
    setIsCartOpen(false);
    setIsReceiptOpen(false);
    setIsPaymentConfirmed(false);
    setIsOrdersModalOpen(false);
    setReceiptData(null);
    setPedidoActual(null);
    setHistorialPedidos([]);
    setPedidoError('');
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const handleOpenOrders = () => {
    setIsOrdersModalOpen(true);
  };

  useEffect(() => {
    try {
      if (isLoggedIn) {
        localStorage.setItem('cafeteria_session', 'true');
      } else {
        localStorage.removeItem('cafeteria_session');
      }
    } catch {
      // Ignorar errores de almacenamiento local.
    }
  }, [isLoggedIn]);

  useEffect(() => {
    if (isLoggedIn) {
      const scrollTimeout = setTimeout(() => {
        document.getElementById('menu')?.scrollIntoView({ behavior: 'smooth', block: 'start' });
      }, 120);

      return () => clearTimeout(scrollTimeout);
    }

    return undefined;
  }, [isLoggedIn]);

  return (
    <>
      <style>{`
        .receipt-overlay,
        .success-overlay {
          position: fixed;
          inset: 0;
          background: rgba(44, 29, 18, 0.42);
          backdrop-filter: blur(5px);
          z-index: 1800;
          display: grid;
          place-items: center;
          padding: 20px;
        }

        .receipt-modal,
        .success-modal {
          width: min(900px, 100%);
          background: #fffdfb;
          border: 1px solid rgba(93, 67, 49, 0.12);
          border-radius: 28px;
          box-shadow: 0 18px 42px rgba(60, 42, 33, 0.2);
          overflow: hidden;
        }

        .receipt-header,
        .success-header {
          padding: 24px 28px 18px;
          border-bottom: 1px solid rgba(93, 67, 49, 0.1);
          background: linear-gradient(180deg, #fdf7f0 0%, #f7eee4 100%);
        }

        .receipt-kicker,
        .success-kicker {
          margin: 0 0 8px;
          color: #8a5a2b;
          font-size: 0.8rem;
          font-weight: 700;
          letter-spacing: 2px;
          text-transform: uppercase;
        }

        .receipt-title,
        .success-title {
          margin: 0;
          color: #3c2a21;
          font-family: 'Playfair Display', serif;
          font-size: clamp(2rem, 3vw, 2.7rem);
        }

        .receipt-body {
          display: grid;
          grid-template-columns: 1.25fr 0.75fr;
          gap: 20px;
          padding: 22px 28px 18px;
          background: #fffdfb;
        }

        .receipt-list {
          display: grid;
          gap: 14px;
        }

        .receipt-item {
          display: grid;
          grid-template-columns: minmax(0, 1.2fr) auto auto auto;
          gap: 12px;
          align-items: center;
          padding: 14px 16px;
          border-radius: 16px;
          background: #fff8f1;
          border: 1px solid #f0d9bf;
        }

        .receipt-item-name {
          margin: 0;
          color: #402d1f;
          font-weight: 700;
        }

        .receipt-item-meta,
        .receipt-item-total {
          color: #6d4f3b;
          font-size: 0.93rem;
          font-weight: 600;
        }

        .receipt-summary {
          background: #f9f4ee;
          border: 1px solid #efddc4;
          border-radius: 18px;
          padding: 18px 18px 14px;
        }

        .receipt-summary-title {
          margin: 0 0 12px;
          color: #3c2a21;
          font-size: 1.1rem;
          font-weight: 800;
        }

        .receipt-detail {
          display: flex;
          justify-content: space-between;
          gap: 12px;
          margin-bottom: 8px;
          color: #5b4233;
          font-size: 0.95rem;
        }

        .receipt-total {
          display: flex;
          justify-content: space-between;
          margin-top: 18px;
          padding-top: 14px;
          border-top: 1px solid rgba(93, 67, 49, 0.12);
          color: #3b2a21;
          font-size: 1.15rem;
          font-weight: 800;
        }

        .receipt-actions {
          display: flex;
          justify-content: flex-end;
          padding: 0 28px 28px;
          background: #fffdfb;
        }

        .receipt-confirm-button,
        .success-button {
          border: none;
          border-radius: 14px;
          padding: 14px 22px;
          background: linear-gradient(135deg, #3c2a21 0%, #5a3d2b 100%);
          color: #fff;
          font-weight: 700;
          cursor: pointer;
          box-shadow: 0 12px 24px rgba(60, 42, 33, 0.18);
        }

        .success-body {
          padding: 24px 28px 28px;
          text-align: center;
          color: #4e392c;
        }

        .success-message {
          margin: 0 0 22px;
          font-size: 1.08rem;
          line-height: 1.6;
        }

        @media (max-width: 780px) {
          .receipt-body {
            grid-template-columns: 1fr;
          }

          .receipt-item {
            grid-template-columns: 1fr;
            text-align: left;
          }
        }
      `}</style>

      <div style={{ fontFamily: 'Arial, sans-serif', margin: 0, padding: 0, color: '#3c2a21' }}>
        <Navbar
          cartCount={cartCount}
          onCartClick={() => setIsCartOpen(true)}
          isLoggedIn={isLoggedIn}
          onLogout={handleLogout}
          onOpenOrders={handleOpenOrders}
        />

        <main>
          {!isLoggedIn ? <Hero onLogin={handleLogin} /> : null}

          {isLoggedIn ? <Menu onAddToCart={handleAddToCart} /> : null}

          {isCartOpen ? (
            <div
              style={{
                position: 'fixed',
                inset: 0,
                background: 'rgba(44, 29, 18, 0.34)',
                backdropFilter: 'blur(4px)',
                zIndex: 1500,
                padding: '20px',
                overflowY: 'auto',
              }}
              onClick={() => setIsCartOpen(false)}
            >
              <div
                style={{
                  maxWidth: '1180px',
                  margin: '22px auto',
                  position: 'relative',
                }}
                onClick={(event) => event.stopPropagation()}
              >
                <button
                  type="button"
                  onClick={() => setIsCartOpen(false)}
                  style={{
                    position: 'absolute',
                    top: '-14px',
                    right: '6px',
                    zIndex: 2,
                    border: 'none',
                    background: '#fff8ef',
                    color: '#3c2a21',
                    borderRadius: '999px',
                    width: '38px',
                    height: '38px',
                    fontSize: '1.1rem',
                    fontWeight: 700,
                    boxShadow: '0 8px 18px rgba(0, 0, 0, 0.16)',
                    cursor: 'pointer',
                  }}
                  aria-label="Cerrar mi pedido"
                >
                  ×
                </button>

                <OrderSection
                  items={cartItems}
                  onRemoveItem={handleRemoveItem}
                  onDecreaseItem={handleDecreaseItem}
                  onIncreaseItem={handleIncreaseItem}
                  onProceedToPayment={handleProceedToPayment}
                  modal
                />
              </div>
            </div>
          ) : null}
        </main>
      </div>

      {isReceiptOpen && receiptData ? (
        <div className="receipt-overlay" onClick={handleCloseReceipt}>
          <div className="receipt-modal" onClick={(event) => event.stopPropagation()}>
            <header className="receipt-header" style={{ position: 'relative' }}>
              <button
                type="button"
                onClick={handleCloseReceipt}
                style={{
                  position: 'absolute',
                  top: '18px',
                  right: '18px',
                  border: 'none',
                  background: '#fff8f2',
                  color: '#3c2a21',
                  width: '36px',
                  height: '36px',
                  borderRadius: '999px',
                  fontSize: '1.2rem',
                  fontWeight: 800,
                  cursor: 'pointer',
                }}
                aria-label="Cerrar resumen de compra"
              >
                ×
              </button>
              <p className="receipt-kicker">Recibo</p>
              <h2 className="receipt-title">Resumen de Orden</h2>
            </header>

            <div className="receipt-body">
              <div className="receipt-list">
                {cartItems.map((item) => (
                  <div key={item.id} className="receipt-item">
                    <div>
                      <p className="receipt-item-name">{item.name}</p>
                    </div>
                    <span className="receipt-item-meta">x{item.quantity}</span>
                    <span className="receipt-item-meta">${item.price}</span>
                    <span className="receipt-item-total">${item.price * item.quantity}</span>
                  </div>
                ))}
              </div>

              <aside className="receipt-summary">
                <h3 className="receipt-summary-title">Datos de pago</h3>
                <div className="receipt-detail">
                  <span>Titular</span>
                  <strong>{receiptData.holder}</strong>
                </div>
                <div className="receipt-detail">
                  <span>Tarjeta</span>
                  <strong>•••• {receiptData.last4}</strong>
                </div>
                <div className="receipt-total">
                  <span>Total a pagar</span>
                  <span>${totalToPay}</span>
                </div>
              </aside>
            </div>

            <div className="receipt-actions" style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-end', gap: '10px' }}>
              {pedidoError ? (
                <p style={{ margin: 0, color: '#8d4638', fontWeight: 700, fontSize: '0.9rem' }}>{pedidoError}</p>
              ) : null}
              <button type="button" className="receipt-confirm-button" onClick={handleConfirmPayment} disabled={isSubmittingOrder} style={{ opacity: isSubmittingOrder ? 0.7 : 1, cursor: isSubmittingOrder ? 'wait' : 'pointer' }}>
                {isSubmittingOrder ? 'Registrando pedido...' : 'Confirmar pago'}
              </button>
            </div>
          </div>
        </div>
      ) : null}

      <PedidosModal
        open={isOrdersModalOpen}
        activePedido={pedidoActual}
        historialPedidos={historialPedidos}
        onClose={() => setIsOrdersModalOpen(false)}
      />

      {isPaymentConfirmed ? (
        <div className="success-overlay" onClick={() => setIsPaymentConfirmed(false)}>
          <div className="success-modal" onClick={(event) => event.stopPropagation()}>
            <header className="success-header">
              <p className="success-kicker">Confirmación</p>
              <h2 className="success-title">¡Gracias por tu compra!</h2>
            </header>
            <div className="success-body">
              <p className="success-message">
                Tu pedido se ha registrado correctamente y el pago fue confirmado con éxito.<br />
                ¡Estamos preparando tu pedido y pronto estará listo!
              </p>
              <button
                type="button"
                className="success-button"
                onClick={() => {
                  setIsPaymentConfirmed(false);
                  setIsOrdersModalOpen(true);
                }}
              >
                Cerrar
              </button>
            </div>
          </div>
        </div>
      ) : null}
    </>
  );
}

export default App;