import logo from '../assets/logo.jpg';
import { useState } from 'react';
import { registrarCuenta, iniciarSesion } from '../services/authService';

export default function Hero({ onLogin = () => {}, onRegister = () => {}, onGuest = () => {} }) {
  const [activePanel, setActivePanel] = useState('');
  const [loginError, setLoginError] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);

  // La cuenta se crea en la base de datos (a través de la API), ya no solo en el navegador.
  const handleRegistration = async (event) => {
    event.preventDefault();
    if (isSubmitting) return;

    const formData = new FormData(event.currentTarget);
    const registeredUser = {
      nombreCompleto: String(formData.get('nombreCompleto') || '').trim(),
      correo: String(formData.get('correo') || '').trim().toLowerCase(),
      anioIngreso: formData.get('anioIngreso'),
      escuela: formData.get('escuela'),
    };

    setIsSubmitting(true);
    setLoginError('');

    try {
      await registrarCuenta({
        nombreCompleto: registeredUser.nombreCompleto,
        correo: registeredUser.correo,
        password: String(formData.get('password') || ''),
        escuela: registeredUser.escuela,
      });

      onRegister(registeredUser, { correo: registeredUser.correo });
      setActivePanel('registro-exitoso');
    } catch (error) {
      setLoginError(error.message);
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleLogin = async (event) => {
    event.preventDefault();
    if (isSubmitting) return;

    const formData = new FormData(event.currentTarget);
    const correo = String(formData.get('loginCorreo') || '').trim().toLowerCase();
    const password = String(formData.get('loginPassword') || '');

    if (!correo || !password) {
      setLoginError('Por favor, ingresa tu correo y contraseña.');
      return;
    }

    setIsSubmitting(true);
    setLoginError('');

    try {
      const usuario = await iniciarSesion({ correo, password });
      setActivePanel('');
      onLogin(usuario);
    } catch (error) {
      setLoginError(error.message);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <>
      <style>{`
        #hero-section {
          background: linear-gradient(135deg, #f6ebdb 0%, #e3d0b5 100%);
          background-image: 
            url(/flowers-pattern.svg),
            linear-gradient(135deg, #f6ebdb 0%, #e3d0b5 100%);
          background-size: 300px 300px, auto;
          background-repeat: repeat, no-repeat;
          background-position: 0 0, center;
          position: relative;
          overflow: hidden;
          isolation: isolate;
        }

        #hero-section::before {
          content: '';
          position: absolute;
          inset: 0;
          background-image: url(${logo});
          background-repeat: no-repeat;
          background-position: center;
          background-size: cover;
          opacity: 0.09;
          pointer-events: none;
          z-index: 0;
        }

        #hero-section > * {
          position: relative;
          z-index: 1;
        }

        .hero-actions {
          display: flex;
          justify-content: center;
          gap: 14px;
          flex-wrap: wrap;
          margin-bottom: 26px;
        }

        .hero-action {
          text-decoration: none;
          padding: 12px 22px;
          border-radius: 999px;
          font-weight: 700;
          box-shadow: 0 6px 16px rgba(60, 42, 33, 0.12);
          transition: transform 0.18s ease, box-shadow 0.18s ease, filter 0.18s ease;
        }

        .hero-action:hover {
          transform: translateY(-1px);
          box-shadow: 0 10px 18px rgba(60, 42, 33, 0.16);
        }

        .hero-action--primary {
          background: linear-gradient(135deg, #3c2a21 0%, #5a3d2b 100%);
          color: #fff;
        }

        .hero-action--secondary {
          background: rgba(255, 249, 241, 0.92);
          color: #3c2a21;
          border: 1px solid rgba(60, 42, 33, 0.18);
        }

        .hero-action--ghost {
          background: rgba(255, 249, 241, 0.4);
          color: #5c4636;
          border: 1px dashed rgba(92, 70, 54, 0.35);
          box-shadow: none;
        }

        .hero-access-grid {
          display: contents;
        }

        .hero-popup-overlay {
          position: fixed;
          inset: 0;
          display: flex;
          align-items: center;
          justify-content: center;
          padding: 24px;
          background: rgba(44, 29, 18, 0.34);
          backdrop-filter: blur(8px);
          z-index: 1400;
        }

        .hero-popup-panel {
          width: min(100%, 980px);
          min-height: min(82vh, 820px);
          text-align: left;
          padding: 0;
          border-radius: 28px;
          background: rgba(255, 251, 246, 0.98);
          border: 1px solid rgba(93, 67, 49, 0.14);
          box-shadow: 0 22px 52px rgba(60, 42, 33, 0.28);
          overflow: hidden;
          display: grid;
          grid-template-columns: 1fr 0.95fr;
        }

        .hero-popup-panel h3 {
          margin: 0 0 8px;
          font-size: 1.6rem;
          color: #3c2a21;
        }

        .hero-popup-panel p {
          margin: 0 0 14px;
          color: #6a5241;
          font-size: 0.98rem;
          line-height: 1.5;
        }

        .hero-modal-visual {
          padding: 28px;
          background:
            radial-gradient(circle at 20% 10%, rgba(255, 244, 228, 0.95) 0%, rgba(255, 244, 228, 0) 42%),
            linear-gradient(180deg, #f8eedf 0%, #f2e2cd 100%);
          border-right: 1px solid rgba(93, 67, 49, 0.1);
          display: flex;
          flex-direction: column;
          justify-content: center;
          gap: 16px;
        }

        .hero-modal-visual-badge {
          width: fit-content;
          padding: 8px 12px;
          border-radius: 999px;
          background: rgba(60, 42, 33, 0.08);
          color: #5c4636;
          font-weight: 700;
          letter-spacing: 0.3px;
        }

        .hero-modal-visual-title {
          margin: 0;
          color: #3c2a21;
          font-family: 'Playfair Display', serif;
          font-size: clamp(2rem, 4vw, 3.4rem);
          line-height: 1.05;
        }

        .hero-modal-visual-text {
          margin: 0;
          color: #6a5241;
          max-width: 420px;
          line-height: 1.65;
        }

        .hero-modal-form-area {
          padding: 28px;
          display: flex;
          flex-direction: column;
          justify-content: center;
        }

        .hero-access-card {
          text-align: left;
          padding: 18px;
          border-radius: 18px;
          background: rgba(255, 251, 246, 0.88);
          border: 1px solid rgba(93, 67, 49, 0.12);
          box-shadow: 0 12px 26px rgba(60, 42, 33, 0.09);
          backdrop-filter: blur(3px);
        }

        .hero-card-actions {
          display: flex;
          justify-content: space-between;
          gap: 10px;
          margin-top: 14px;
          flex-wrap: wrap;
        }

        .hero-card-switch {
          border: 1px solid #e2c8a9;
          background: #fff8ef;
          color: #6a4a32;
          border-radius: 10px;
          padding: 9px 12px;
          font-weight: 700;
          cursor: pointer;
        }

        .hero-card-close {
          border: none;
          background: transparent;
          color: #8a5a2b;
          font-weight: 700;
          cursor: pointer;
          padding: 0;
        }

        .hero-access-card h3 {
          margin: 0 0 8px;
          font-size: 1rem;
          color: #3c2a21;
        }

        .hero-access-card p {
          margin: 0 0 14px;
          color: #6a5241;
          font-size: 0.94rem;
          line-height: 1.5;
        }

        .hero-mini-form {
          display: grid;
          gap: 10px;
        }

        .hero-form-error {
          margin: 0;
          padding: 10px 12px;
          border: 1px solid #d9a69b;
          border-radius: 10px;
          background: #fff0ec;
          color: #8d4638;
          font-size: 0.9rem;
          line-height: 1.4;
        }

        .hero-mini-input {
          width: 100%;
          box-sizing: border-box;
          border: 1px solid #e2c8a9;
          border-radius: 12px;
          padding: 11px 12px;
          font-size: 0.95rem;
          background: #fffdfb;
          color: #4a3020;
          outline: none;
        }

        .hero-mini-input:focus {
          border-color: #a66b3d;
          box-shadow: 0 0 0 3px rgba(166, 107, 61, 0.14);
        }

        .hero-mini-select {
          width: 100%;
          box-sizing: border-box;
          border: 1px solid #e2c8a9;
          border-radius: 12px;
          padding: 11px 12px;
          font-size: 0.95rem;
          background: #fffdfb;
          color: #4a3020;
          outline: none;
        }

        .hero-mini-select:focus {
          border-color: #a66b3d;
          box-shadow: 0 0 0 3px rgba(166, 107, 61, 0.14);
        }

        .hero-mini-button {
          border: none;
          border-radius: 12px;
          padding: 11px 14px;
          font-weight: 700;
          cursor: pointer;
          background: #3c2a21;
          color: #fff;
        }

        .hero-mini-button--light {
          background: #fff4e7;
          color: #5c4636;
          border: 1px solid #e5c9a6;
        }

        .hero-guest-badge {
          display: inline-flex;
          align-items: center;
          gap: 8px;
          margin-bottom: 10px;
          padding: 7px 10px;
          border-radius: 999px;
          background: rgba(60, 42, 33, 0.08);
          color: #5c4636;
          font-size: 0.88rem;
          font-weight: 700;
        }

        @media (max-width: 860px) {
          .hero-popup-overlay {
            padding: 14px;
            align-items: flex-start;
            overflow-y: auto;
          }

          .hero-popup-panel {
            grid-template-columns: 1fr;
            min-height: auto;
            border-radius: 24px;
          }

          .hero-modal-visual {
            border-right: none;
            border-bottom: 1px solid rgba(93, 67, 49, 0.1);
          }
        }
      `}</style>
      <section
        id="hero-section"
        style={{
          padding: '90px 24px',
          textAlign: 'center',
          minHeight: '78vh',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center'
      }}
    >
      <div style={{ maxWidth: '900px', width: '100%' }}>
        <p style={{ margin: '0 0 12px', color: '#8a5a2b', fontWeight: '700', letterSpacing: '2px', textTransform: 'uppercase', fontSize: '13px' }}>
          Cafetería • Pick-up rápido
        </p>
        <h1 style={{ margin: '0 0 16px', fontSize: 'clamp(2.2rem, 4vw, 3.5rem)', lineHeight: 1.1, color: '#3c2a21', fontFamily: 'Georgia, serif' }}>
          El mejor servicio para empezar tu día con sabor.
        </h1>
        <p style={{ margin: '0 auto 28px', maxWidth: '680px', fontSize: '1.08rem', lineHeight: 1.7, color: '#5c4636' }}>
          Disfruta de un rico desayuno o una deliciosa comida y un servicio ágil pensado para que tu pedido llegue perfecto a tu rutina.
        </p>

        <div className="hero-actions">
          <a
            href="#registro"
            className="hero-action hero-action--primary"
            onClick={(event) => {
              event.preventDefault();
              setActivePanel('registro');
            }}
          >
            Registrame
          </a>

          <a
            href="#login"
            className="hero-action hero-action--secondary"
            onClick={(event) => {
              event.preventDefault();
              setActivePanel('login');
            }}
          >
            Iniciar sesion
          </a>

          <a
            href="#invitado"
            className="hero-action hero-action--ghost"
            onClick={(event) => {
              event.preventDefault();
              setActivePanel('invitado');
            }}
          >
            Invitado
          </a>
        </div>

        {activePanel ? (
          <div className="hero-popup-overlay" onClick={() => setActivePanel('')}>
            <section
              className="hero-popup-panel"
              id={activePanel}
              onClick={(event) => event.stopPropagation()}
            >
              {activePanel === 'registro' ? (
                <>
                  <div className="hero-modal-visual">
                    <div className="hero-modal-visual-badge">Registro escolar</div>
                    <h3 className="hero-modal-visual-title">Crea tu cuenta y deja listo tu perfil</h3>
                    <p className="hero-modal-visual-text">
                      Guarda tus pedidos, recibe promos y vuelve a pedir en segundos.
                    </p>
                    <p className="hero-modal-visual-text">
                      Queremos brindarte la mejor experiencia, por eso necesitamos algunos datos para crear tu perfil.
                    </p>
                  </div>
                  <div className="hero-modal-form-area">
                    <h3>Crear cuenta</h3>
                    <p>Llena estos datos para registrar tu acceso.</p>
                    <form className="hero-mini-form" onSubmit={handleRegistration}>
                      <input className="hero-mini-input" name="nombreCompleto" type="text" placeholder="Nombre completo" autoComplete="name" required />
                      <input className="hero-mini-input" name="correo" type="email" placeholder="Correo institucional" autoComplete="email" required />
                      <input className="hero-mini-input" name="anioIngreso" type="number" min="1960" max="2035" placeholder="Año en que entro al Poli" required />
                      <select className="hero-mini-select" name="escuela" defaultValue="" required>
                        <option value="" disabled>
                          Escuela de procedencia del Poli
                        </option>
                        <option value="upiita">UPIITA</option>
                        <option value="esime">ESIME</option>
                        <option value="esca">ESCA</option>
                        <option value="esfm">ESFM</option>
                        <option value="encb">ENCB</option>
                        <option value="upiih">UPIIH</option>
                        <option value="cecyt">CECyT 9</option>
                        <option value="otra">Otra</option>
                      </select>
                      <input className="hero-mini-input" name="password" type="password" placeholder="Crear contraseña" minLength="6" required />
                      {loginError ? <p className="hero-form-error" role="alert">{loginError}</p> : null}
                      <button type="submit" className="hero-mini-button" disabled={isSubmitting}>
                        {isSubmitting ? 'Creando cuenta...' : 'Registrarme'}
                      </button>
                    </form>
                    <div className="hero-card-actions">
                      <button type="button" className="hero-card-close" onClick={() => setActivePanel('')}>
                        Cerrar
                      </button>
                    </div>
                  </div>
                </>
              ) : null}

              {activePanel === 'registro-exitoso' ? (
                <>
                  <div className="hero-modal-visual">
                    <div className="hero-modal-visual-badge">Registro completado</div>
                    <h3 className="hero-modal-visual-title">Tu cuenta está lista</h3>
                    <p className="hero-modal-visual-text">
                      Ya puedes iniciar sesión para entrar al menú y empezar a preparar tu pedido.
                    </p>
                  </div>
                  <div className="hero-modal-form-area">
                    <h3>¡Registro exitoso!</h3>
                    <p>Tu cuenta ha sido creada correctamente.</p>
                    <button
                      type="button"
                      className="hero-mini-button"
                      onClick={() => setActivePanel('login')}
                    >
                      Iniciar sesión
                    </button>
                    <div className="hero-card-actions">
                      <button type="button" className="hero-card-close" onClick={() => setActivePanel('')}>
                        Cerrar
                      </button>
                    </div>
                  </div>
                </>
              ) : null}

              {activePanel === 'login' ? (
                <>
                  <div className="hero-modal-visual">
                    <div className="hero-modal-visual-badge">Acceso rapido</div>
                    <h3 className="hero-modal-visual-title">Entra y sigue tu pedido sin perder tu historial</h3>
                    <p className="hero-modal-visual-text">
                      El servicio más eficiente para gestionar tu pedido. <br />¡Inicia sesión y disfruta de la experiencia Upiifood!
                    </p>
                    <p className="hero-modal-visual-text">
                      Al ingresar te llevamos directo al menú.
                    </p>
                  </div>
                  <div className="hero-modal-form-area">
                    <h3>Iniciar sesión</h3>
                    <p>Usa tus datos para entrar al sistema.</p>
                    <form className="hero-mini-form" onSubmit={handleLogin} noValidate>
                      <input className="hero-mini-input" name="loginCorreo" type="email" placeholder="Correo" autoComplete="email" aria-describedby={loginError ? 'login-error' : undefined} />
                      <input className="hero-mini-input" name="loginPassword" type="password" placeholder="Contraseña" autoComplete="current-password" aria-describedby={loginError ? 'login-error' : undefined} />
                      {loginError ? <p className="hero-form-error" id="login-error" role="alert">{loginError}</p> : null}
                      <button type="submit" className="hero-mini-button hero-mini-button--light">
                        Ingresar
                      </button>
                    </form>
                    <div className="hero-card-actions">
                      <button type="button" className="hero-card-close" onClick={() => setActivePanel('')}>
                        Cerrar
                      </button>
                      <button type="button" className="hero-card-switch" onClick={() => setActivePanel('registro')}>
                        Crear cuenta
                      </button>
                    </div>
                  </div>
                </>
              ) : null}

              {activePanel === 'invitado' ? (
                <>
                  <div className="hero-modal-visual">
                    <div className="hero-modal-visual-badge">Invitado</div>
                    <h3 className="hero-modal-visual-title">Explora el menu sin registro</h3>
                    <p className="hero-modal-visual-text">
                      Podrás ver el menú, agregar productos y revisar tu pedido con una ventana grande y elegante.
                    </p>
                    <p className="hero-modal-visual-text">
                      Animate a registrarte y disfrutar de más beneficios.
                    </p>
                  </div>
                  <div className="hero-modal-form-area">
                    <h3>Entrar como invitado</h3>
                    <p>Accede rápido al menú y arma tu pedido.</p>
                    <button
                      type="button"
                      className="hero-mini-button"
                      onClick={() => {
                        setActivePanel('');
                        onGuest();
                      }}
                    >
                      Ir al menu
                    </button>
                    <div className="hero-card-actions">
                      <button type="button" className="hero-card-close" onClick={() => setActivePanel('')}>
                        Cerrar
                      </button>
                      <button type="button" className="hero-card-switch" onClick={() => setActivePanel('registro')}>
                        Quiero registrarme
                      </button>
                    </div>
                  </div>
                </>
              ) : null}
            </section>
          </div>
        ) : null}
      </div>
    </section>
    </>
  );
}