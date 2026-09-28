import { useState } from 'react';

export default function OrderSection({
  items = [],
  onRemoveItem = () => {},
  onDecreaseItem = () => {},
  onIncreaseItem = () => {},
  onProceedToPayment = () => {},
  guestMode = false,
  onGuestConfirmOrder = () => {},
  modal = false,
  paymentMethod = null,
}) {
  const total = items.reduce((sum, item) => sum + item.price * item.quantity, 0);
  const [paymentForm, setPaymentForm] = useState({
    holder: '',
    cardNumber: '',
    expiry: '',
    cvv: '',
    postal: '',
  });
  const [paymentError, setPaymentError] = useState('');
  const [guestPaymentError, setGuestPaymentError] = useState('');

  const handleInputChange = (event) => {
    const { name, value } = event.target;

    let nextValue = value;

    if (name === 'cardNumber') {
      nextValue = value.replace(/\D/g, '').slice(0, 16).replace(/(.{4})/g, '$1 ').trim();
    }

    if (name === 'expiry') {
      nextValue = value
        .replace(/\D/g, '')
        .slice(0, 4)
        .replace(/^(\d{2})(\d{0,2})$/, (_, month, year) => (year ? `${month}/${year}` : month));
    }

    if (name === 'cvv') {
      nextValue = value.replace(/\D/g, '').slice(0, 3);
    }

    if (name === 'postal') {
      nextValue = value.replace(/\D/g, '').slice(0, 5);
    }

    if (paymentError) setPaymentError('');

    setPaymentForm((current) => ({
      ...current,
      [name]: nextValue,
    }));
  };

  const handleProceedToPayment = () => {
    if (!items.length) return;

    if (!paymentForm.holder.trim()) {
      setPaymentError('Ingresa el nombre que aparece en la tarjeta.');
      return;
    }

    const cardDigits = paymentForm.cardNumber.replace(/\D/g, '');
    if (!/^\d{16}$/.test(cardDigits)) {
      setPaymentError('El número de tarjeta debe tener 16 dígitos.');
      return;
    }

    if (!/^(0[1-9]|1[0-2])\/\d{2}$/.test(paymentForm.expiry)) {
      setPaymentError('Ingresa una fecha válida con formato MM/AA.');
      return;
    }

    if (!/^\d{3}$/.test(paymentForm.cvv)) {
      setPaymentError('El CVV debe tener exactamente 3 dígitos.');
      return;
    }

    setPaymentError('');

    onProceedToPayment({
      holder: paymentForm.holder.trim(),
      last4: cardDigits.slice(-4),
      expiry: paymentForm.expiry,
    });
  };

  const handleGuestPaymentSubmit = (event) => {
    event.preventDefault();
    if (!items.length) return;

    if (!paymentForm.holder.trim()) {
      setGuestPaymentError('Ingresa el nombre que aparece en la tarjeta.');
      return;
    }

    if (!/^\d{16}$/.test(paymentForm.cardNumber.replace(/\s/g, ''))) {
      setGuestPaymentError('El número de tarjeta debe tener 16 dígitos.');
      return;
    }

    if (!/^(0[1-9]|1[0-2])\/\d{2}$/.test(paymentForm.expiry)) {
      setGuestPaymentError('Ingresa una fecha válida con formato MM/AA.');
      return;
    }

    if (!/^\d{3}$/.test(paymentForm.cvv)) {
      setGuestPaymentError('El CVV debe tener exactamente 3 dígitos.');
      return;
    }

    setGuestPaymentError('');
    // Los datos de tarjeta solo se validan en este formulario y nunca se envían ni persisten.
    onGuestConfirmOrder();
  };

  return (
    <>
      <style>{`
        .order-section {
          padding: 72px 22px 84px;
          background:
            radial-gradient(circle at 85% 0%, rgba(255, 244, 228, 0.9) 0%, rgba(255, 244, 228, 0) 35%),
            linear-gradient(180deg, #f7ead6 0%, #edd8bd 100%);
        }

        .order-section--modal {
          padding: 0;
          background: transparent;
        }

        .order-wrap {
          max-width: 1180px;
          margin: 0 auto;
          display: grid;
          grid-template-columns: 1.1fr 0.9fr;
          gap: 22px;
          align-items: start;
        }

        .order-wrap--modal {
          max-width: 100%;
        }

        .order-panel,
        .payment-panel {
          background: rgba(255, 251, 246, 0.95);
          border: 1px solid rgba(93, 67, 49, 0.12);
          border-radius: 24px;
          box-shadow: 0 14px 32px rgba(72, 48, 31, 0.12);
          overflow: hidden;
        }

        .order-header,
        .payment-header {
          padding: 22px 22px 18px;
          border-bottom: 1px solid rgba(93, 67, 49, 0.1);
        }

        .order-kicker {
          margin: 0 0 8px;
          color: #8a5a2b;
          font-size: 0.8rem;
          font-weight: 700;
          letter-spacing: 2px;
          text-transform: uppercase;
        }

        .order-title {
          margin: 0;
          color: #3c2a21;
          font-family: 'Playfair Display', serif;
          font-size: clamp(1.8rem, 3vw, 2.5rem);
        }

        .order-subtitle {
          margin: 10px 0 0;
          color: #6d4f3b;
          font-size: 0.95rem;
          line-height: 1.5;
        }

        .order-list {
          padding: 18px 18px 20px;
          display: grid;
          gap: 14px;
        }

        .order-empty {
          padding: 28px 18px 32px;
          color: #6d4f3b;
          text-align: center;
        }

        .order-item {
          display: grid;
          grid-template-columns: 86px 1fr auto;
          gap: 14px;
          align-items: center;
          padding: 14px;
          border-radius: 18px;
          background: #fff8f0;
          border: 1px solid #f0d7b9;
        }

        .order-thumb {
          width: 86px;
          height: 86px;
          border-radius: 16px;
          object-fit: cover;
          box-shadow: 0 8px 16px rgba(72, 48, 31, 0.12);
        }

        .order-item-info {
          min-width: 0;
        }

        .order-item-name {
          margin: 0 0 4px;
          color: #3f2816;
          font-size: 1.02rem;
          font-weight: 700;
        }

        .order-item-meta {
          margin: 0;
          color: #76543d;
          font-size: 0.92rem;
          line-height: 1.45;
        }

        .order-item-actions {
          display: flex;
          flex-direction: column;
          align-items: flex-end;
          gap: 8px;
        }

        .order-quantity-row {
          display: inline-flex;
          align-items: center;
          gap: 8px;
        }

        .order-quantity-btn {
          border: none;
          width: 34px;
          height: 34px;
          border-radius: 999px;
          background: #fff3e7;
          color: #6a4328;
          font-weight: 800;
          cursor: pointer;
          box-shadow: inset 0 0 0 1px #e2c5a2;
        }

        .order-quantity-btn:disabled {
          opacity: 0.45;
          cursor: not-allowed;
        }

        .order-item-price {
          color: #9d4f1e;
          font-weight: 800;
          font-size: 1rem;
        }

        .order-remove-btn {
          border: none;
          background: rgba(141, 70, 56, 0.1);
          color: #8d4638;
          font-weight: 700;
          border-radius: 999px;
          padding: 8px 12px;
          cursor: pointer;
        }

        .order-summary {
          padding: 18px 22px 22px;
          border-top: 1px solid rgba(93, 67, 49, 0.1);
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 12px;
        }

        .order-total-label {
          margin: 0;
          color: #6d4f3b;
          font-size: 0.92rem;
        }

        .order-total-value {
          margin: 4px 0 0;
          color: #3c2a21;
          font-size: 1.5rem;
          font-weight: 800;
        }

        .order-note {
          margin: 0;
          color: #7a5b45;
          font-size: 0.9rem;
          text-align: right;
        }

        .payment-body {
          padding: 18px 22px 22px;
          display: grid;
          gap: 12px;
        }

        .payment-badge {
          display: inline-flex;
          align-items: center;
          gap: 8px;
          width: fit-content;
          padding: 8px 12px;
          border-radius: 999px;
          background: rgba(60, 42, 33, 0.08);
          color: #5c4636;
          font-size: 0.84rem;
          font-weight: 700;
        }

        .payment-form {
          display: grid;
          gap: 10px;
        }

        .payment-input {
          width: 100%;
          box-sizing: border-box;
          border: 1px solid #e1c7a6;
          border-radius: 14px;
          padding: 12px 14px;
          background: #fffdfb;
          color: #4a3020;
          font-size: 0.95rem;
          outline: none;
        }

        .payment-input:focus {
          border-color: #a66b3d;
          box-shadow: 0 0 0 3px rgba(166, 107, 61, 0.14);
        }

        .payment-error {
          grid-column: 1 / -1;
          margin: -4px 0 0;
          color: #8d4638;
          font-size: 0.82rem;
          font-weight: 600;
        }

        .saved-payment-card {
          display: grid;
          gap: 7px;
          padding: 16px;
          border: 1px solid #e7c9a3;
          border-radius: 14px;
          background: linear-gradient(135deg, #fff9f2 0%, #f5e8d8 100%);
          color: #4a3020;
        }

        .saved-payment-number {
          color: #3c2a21;
          font-size: 1.15rem;
          font-weight: 800;
          letter-spacing: 1px;
        }

        .payment-grid {
          display: grid;
          grid-template-columns: 1.7fr 0.8fr 0.6fr;
          gap: 10px;
        }

        .payment-button {
          border: none;
          border-radius: 14px;
          padding: 13px 16px;
          background: linear-gradient(135deg, #3c2a21 0%, #5a3d2b 100%);
          color: #fff;
          font-weight: 700;
          cursor: pointer;
          box-shadow: 0 10px 20px rgba(60, 42, 33, 0.18);
        }

        .payment-small-note {
          margin: 0;
          color: #7a5b45;
          font-size: 0.9rem;
          line-height: 1.5;
        }

        @media (max-width: 900px) {
          .order-wrap {
            grid-template-columns: 1fr;
          }

          .payment-grid {
            grid-template-columns: 1fr;
          }
        }

        @media (max-width: 640px) {
          .order-item {
            grid-template-columns: 1fr;
          }

          .order-item-actions {
            align-items: flex-start;
          }

          .order-summary {
            flex-direction: column;
            align-items: flex-start;
          }

          .order-note {
            text-align: left;
          }
        }
      `}</style>

      <section id="pedido" className={`order-section ${modal ? 'order-section--modal' : ''}`}>
        <div className={`order-wrap ${modal ? 'order-wrap--modal' : ''}`}>
          <div className="order-panel">
            <header className="order-header">
              <p className="order-kicker">Tu pedido</p>
              <h2 className="order-title">Revisa lo que seleccionaste</h2>
             
            </header>

            {items.length > 0 ? (
              <div className="order-list">
                {items.map((item) => (
                  <article key={item.id} className="order-item">
                    <img src={item.image} alt={item.name} className="order-thumb" />
                    <div className="order-item-info">
                      <h3 className="order-item-name">{item.name}</h3>
                      <p className="order-item-meta">
                        Cantidad: {item.quantity} <br />
                        Precio unitario: ${item.price}
                      </p>
                    </div>
                    <div className="order-item-actions">
                      <span className="order-item-price">${item.price * item.quantity}</span>
                      <div className="order-quantity-row">
                        <button
                          type="button"
                          className="order-quantity-btn"
                          onClick={() => onDecreaseItem(item.id)}
                          disabled={item.quantity <= 1}
                          aria-label={`Disminuir cantidad de ${item.name}`}
                        >
                          -
                        </button>
                        <span style={{ color: '#6d4f3b', fontWeight: 700, minWidth: '24px', textAlign: 'center' }}>
                          {item.quantity}
                        </span>
                        <button
                          type="button"
                          className="order-quantity-btn"
                          onClick={() => onIncreaseItem(item.id)}
                          aria-label={`Aumentar cantidad de ${item.name}`}
                        >
                          +
                        </button>
                      </div>
                      <button type="button" className="order-remove-btn" onClick={() => onRemoveItem(item.id)}>
                        Eliminar
                      </button>
                    </div>
                  </article>
                ))}
              </div>
            ) : (
              <div className="order-empty">
                Aun no has agregado productos.
              </div>
            )}

            <div className="order-summary">
              <div>
                <p className="order-total-label">Total a pagar</p>
                <p className="order-total-value">${total}</p>
              </div>
            </div>
          </div>

          <div className="payment-panel">
            <header className="payment-header">
              <p className="order-kicker">Pago seguro</p>
              <h2 className="order-title">Proceder al pago</h2>
            </header>

            <div className="payment-body">
              {guestMode ? (
                <>
                  <div className="payment-badge">Pedido como invitado</div>
                  <p className="payment-small-note">Ingresa los datos de tu tarjeta para validar el pago simulado de este pedido. No se guardarán ni se asociarán a una cuenta.</p>
                  <form className="payment-form" onSubmit={handleGuestPaymentSubmit}>
                    <input
                      className="payment-input"
                      type="text"
                      name="holder"
                      placeholder="Nombre en la tarjeta"
                      autoComplete="cc-name"
                      value={paymentForm.holder}
                      onChange={handleInputChange}
                      required
                    />
                    <input
                      className="payment-input"
                      type="text"
                      name="cardNumber"
                      placeholder="Número de tarjeta"
                      inputMode="numeric"
                      autoComplete="cc-number"
                      maxLength={19}
                      value={paymentForm.cardNumber}
                      onChange={handleInputChange}
                      required
                    />
                    <div className="payment-grid">
                      <input
                        className="payment-input"
                        type="text"
                        name="expiry"
                        placeholder="MM/AA"
                        inputMode="numeric"
                        autoComplete="cc-exp"
                        maxLength={5}
                        pattern="(0[1-9]|1[0-2])/[0-9]{2}"
                        value={paymentForm.expiry}
                        onChange={handleInputChange}
                        required
                      />
                      <input
                        className="payment-input"
                        type="text"
                        name="cvv"
                        placeholder="CVV"
                        inputMode="numeric"
                        autoComplete="cc-csc"
                        pattern="[0-9]{3}"
                        maxLength={3}
                        value={paymentForm.cvv}
                        onChange={handleInputChange}
                        required
                      />
                    </div>
                    {guestPaymentError ? <p className="payment-error" role="alert">{guestPaymentError}</p> : null}
                    <button
                      type="submit"
                      className="payment-button"
                      disabled={!items.length}
                      style={{ opacity: items.length ? 1 : 0.55, cursor: items.length ? 'pointer' : 'not-allowed' }}
                    >
                      Confirmar pedido
                    </button>
                  </form>
                </>
              ) : paymentMethod ? (
                <>
                  <div className="payment-badge">Método de pago guardado</div>
                  <div className="saved-payment-card">
                    <strong>{paymentMethod.holder}</strong>
                    <span className="saved-payment-number">•••• {paymentMethod.last4}</span>
                    <span>Expira {paymentMethod.expiry}</span>
                  </div>
                  <p className="payment-small-note">Confirma tu pedido con tu método principal guardado.</p>
                  <button
                    type="button"
                    className="payment-button"
                    onClick={() => onProceedToPayment({
                      holder: paymentMethod.holder,
                      last4: paymentMethod.last4,
                      expiry: paymentMethod.expiry,
                      savedPaymentMethod: true,
                    })}
                    disabled={!items.length}
                    style={{ opacity: items.length ? 1 : 0.55, cursor: items.length ? 'pointer' : 'not-allowed' }}
                  >
                    Confirmar pedido
                  </button>
                </>
              ) : (
                <>
              <div className="payment-badge">Solo pago con tarjeta</div>
              <form className="payment-form" onSubmit={(event) => event.preventDefault()}>
                <input
                  className="payment-input"
                  type="text"
                  name="holder"
                  placeholder="Nombre del titular"
                  value={paymentForm.holder}
                  onChange={handleInputChange}
                />
                <input
                  className="payment-input"
                  type="text"
                  name="cardNumber"
                  placeholder="Numero de tarjeta"
                  maxLength="19"
                  value={paymentForm.cardNumber}
                  onChange={handleInputChange}
                />
                <div className="payment-grid">
                  <input
                    className="payment-input"
                    type="text"
                    name="expiry"
                    placeholder="MM/AA"
                    maxLength="5"
                    value={paymentForm.expiry}
                    onChange={handleInputChange}
                  />
                  <input
                    className="payment-input"
                    type="text"
                    name="cvv"
                    placeholder="CVV"
                    inputMode="numeric"
                    pattern="[0-9]{3}"
                    maxLength={3}
                    value={paymentForm.cvv}
                    onChange={handleInputChange}
                    aria-invalid={Boolean(paymentError)}
                    aria-describedby={paymentError ? 'payment-form-error' : undefined}
                  />
                  <input
                    className="payment-input"
                    type="text"
                    name="postal"
                    placeholder="Postal"
                    maxLength="5"
                    value={paymentForm.postal}
                    onChange={handleInputChange}
                  />
                </div>
                {paymentError ? <p className="payment-error" id="payment-form-error" role="alert">{paymentError}</p> : null}
                <button type="button" className="payment-button" onClick={handleProceedToPayment}>
                  Proceder al pago
                </button>
              </form>
              <p className="payment-small-note">
                La compra se procesara con tarjeta bancaria o tarjeta de debito. 
              </p>
                </>
              )}
            </div>
          </div>
        </div>
      </section>
    </>
  );
}
