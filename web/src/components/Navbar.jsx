import React, { useState } from 'react';
import logo from '../assets/logo.jpg';

export default function Navbar({ cartCount = 0, onCartClick = () => {}, isLoggedIn = false, onLogout = () => {}, onOpenOrders = () => {} }) {
  const [profileMenuOpen, setProfileMenuOpen] = useState(false);

  return (
    <>
      <style>{`
        @import url('https://fonts.googleapis.com/css2?family=Parisienne&family=Playfair+Display:wght@600;700&family=Inter:wght@400;500;600&display=swap');

        .navbar-shell {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding: 15px 40px;
          background: linear-gradient(90deg, #2f2018 0%, #493223 100%);
          color: #fff;
          position: sticky;
          top: 0;
          z-index: 1000;
          box-shadow: 0 6px 18px rgba(0, 0, 0, 0.15);
        }

        .navbar-brand {
          display: flex;
          align-items: center;
          gap: 10px;
          font-family: 'Playfair Display', serif;
          font-size: 24px;
          font-weight: 700;
          letter-spacing: 0.8px;
        }

        .brand-text {
          font-family: 'Parisienne', cursive;
          font-style: normal;
          font-weight: 400;
          font-size: 50px;
          line-height: 1;
          letter-spacing: 0.3px;
          color: #f8efe2;
          text-shadow: 0 2px 8px rgba(0, 0, 0, 0.22);
          transform: translateY(1px);
          white-space: nowrap;
        }

        .brand-icon {
          display: inline-flex;
          align-items: center;
          justify-content: center;
          width: 56px;
          height: 56px;
          border-radius: 50%;
          background: #ffffff;
          border: 1px solid rgba(255, 255, 255, 0.65);
          box-shadow: 0 3px 10px rgba(0, 0, 0, 0.18);
          overflow: hidden;
          flex-shrink: 0;
          padding: 1px;
        }

        .brand-icon img {
          width: 128%;
          height: 128%;
          object-fit: cover;
          transform: scale(1.06);
          border-radius: 50%;
        }

        .nav-links {
          display: flex;
          gap: 22px;
        }

        .nav-link {
          color: #efe4d4;
          text-decoration: none;
          font-family: 'Inter', sans-serif;
          font-weight: 500;
          transition: color 0.2s ease, transform 0.2s ease;
        }

        .nav-link:hover {
          color: #f1c98b;
          transform: translateY(-1px);
        }

        .cart-button {
          appearance: none;
          -webkit-appearance: none;
          position: relative;
          cursor: pointer;
          background: #5a3d2b;
          padding: 8px 15px;
          border-radius: 999px;
          display: flex;
          align-items: center;
          gap: 8px;
          border: 1px solid #7d5a44;
          box-shadow: 0 4px 10px rgba(0, 0, 0, 0.14);
          transition: transform 0.2s ease, box-shadow 0.2s ease;
        }

        .cart-button:hover {
          transform: translateY(-2px);
          box-shadow: 0 8px 16px rgba(0, 0, 0, 0.18);
        }

        .cart-badge {
          background: #d4a373;
          color: #3c2a21;
          border-radius: 50%;
          padding: 2px 7px;
          font-size: 12px;
          font-weight: 700;
        }

        .profile-button {
          position: relative;
          cursor: pointer;
          background: rgba(255, 248, 239, 0.14);
          padding: 8px 14px 8px 10px;
          border-radius: 999px;
          display: flex;
          align-items: center;
          gap: 10px;
          border: 1px solid rgba(255, 248, 239, 0.22);
          box-shadow: 0 4px 10px rgba(0, 0, 0, 0.12);
          transition: transform 0.2s ease, box-shadow 0.2s ease, background 0.2s ease;
          color: #fff8ef;
        }

        .profile-wrapper {
          position: relative;
        }

        .profile-button:hover {
          transform: translateY(-2px);
          background: rgba(255, 248, 239, 0.18);
          box-shadow: 0 8px 16px rgba(0, 0, 0, 0.16);
        }

        .profile-dropdown {
          position: absolute;
          top: calc(100% + 10px);
          right: 0;
          min-width: 220px;
          padding: 10px;
          border-radius: 18px;
          background: rgba(255, 251, 246, 0.98);
          border: 1px solid rgba(93, 67, 49, 0.14);
          box-shadow: 0 18px 36px rgba(60, 42, 33, 0.22);
          z-index: 1200;
          backdrop-filter: blur(4px);
        }

        .profile-dropdown-header {
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 8px 10px 12px;
          border-bottom: 1px solid rgba(93, 67, 49, 0.1);
          margin-bottom: 8px;
        }

        .profile-dropdown-title {
          display: flex;
          flex-direction: column;
          line-height: 1.1;
        }

        .profile-dropdown-title strong {
          color: #3c2a21;
          font-size: 0.98rem;
        }

        .profile-dropdown-title span {
          color: #7a5b45;
          font-size: 0.85rem;
        }

        .profile-option {
          width: 100%;
          border: none;
          background: transparent;
          color: #4a3020;
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 10px;
          padding: 11px 12px;
          border-radius: 12px;
          cursor: pointer;
          font-weight: 600;
          font-size: 0.94rem;
          text-align: left;
          transition: background 0.18s ease, transform 0.18s ease;
        }

        .profile-option:hover {
          background: #fff4e7;
          transform: translateX(2px);
        }

        .profile-option-destructive {
          color: #8d4638;
        }

        .profile-avatar {
          width: 34px;
          height: 34px;
          border-radius: 50%;
          background: linear-gradient(135deg, #f6ebdb 0%, #d4a373 100%);
          color: #3c2a21;
          display: inline-flex;
          align-items: center;
          justify-content: center;
          font-weight: 800;
          font-size: 14px;
          box-shadow: inset 0 1px 2px rgba(255, 255, 255, 0.45);
        }

        .profile-labels {
          display: flex;
          flex-direction: column;
          line-height: 1.05;
          text-align: left;
        }

        .profile-labels span:first-child {
          font-size: 0.72rem;
          letter-spacing: 0.8px;
          text-transform: uppercase;
          color: rgba(255, 248, 239, 0.72);
        }

        .profile-labels span:last-child {
          font-size: 0.95rem;
          font-weight: 700;
          color: #fff8ef;
        }
      `}</style>

      <nav className="navbar-shell">
        <div className="navbar-brand">
          <span className="brand-icon" aria-hidden="true">
            <img src={logo} alt="Logo UPIIFOOD" />
          </span>
          <span className="brand-text">Upiifood</span>
        </div>

        <div className="nav-links">
          <a href="#inicio" className="nav-link">Inicio</a>
          <a href="#menu" className="nav-link">Menú</a>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '12px', flexWrap: 'wrap', justifyContent: 'flex-end' }}>
          {isLoggedIn ? (
            <>
              <button type="button" className="cart-button" onClick={onCartClick} aria-label="Abrir mi pedido">
                <span>🛒</span>
                <span style={{ fontWeight: '700' }}>Mi Pedido</span>
                <span className="cart-badge">{cartCount}</span>
              </button>

              <div
                className="profile-wrapper"
                onBlur={(event) => {
                  if (!event.currentTarget.contains(event.relatedTarget)) {
                    setProfileMenuOpen(false);
                  }
                }}
              >
                <div
                  className="profile-button"
                  role="button"
                  tabIndex="0"
                  aria-label="Perfil del usuario"
                  aria-expanded={profileMenuOpen}
                  onClick={() => setProfileMenuOpen((value) => !value)}
                  onKeyDown={(event) => {
                    if (event.key === 'Enter' || event.key === ' ') {
                      event.preventDefault();
                      setProfileMenuOpen((value) => !value);
                    }
                  }}
                >
                  <span className="profile-avatar" aria-hidden="true">U</span>
                  <span className="profile-labels">
                    <span>Perfil</span>
                    <span>Usuario</span>
                  </span>
                </div>

                {profileMenuOpen ? (
                  <div className="profile-dropdown" role="menu" aria-label="Opciones de perfil">
                    <div className="profile-dropdown-header">
                      <div className="profile-avatar" aria-hidden="true">U</div>
                      <div className="profile-dropdown-title">
                        <strong>Usuario</strong>
                        <span>Cuenta activa</span>
                      </div>
                    </div>

                    <button
                      type="button"
                      className="profile-option"
                      onClick={() => {
                        setProfileMenuOpen(false);
                        onOpenOrders();
                      }}
                    >
                      <span>Mis pedidos</span>
                      <span>›</span>
                    </button>

                    <button
                      type="button"
                      className="profile-option"
                      onClick={() => setProfileMenuOpen(false)}
                    >
                      <span>Configuración</span>
                      <span>›</span>
                    </button>

                    <button
                      type="button"
                      className="profile-option profile-option-destructive"
                      onClick={() => {
                        setProfileMenuOpen(false);
                        onLogout();
                      }}
                    >
                      <span>Cerrar sesión</span>
                      <span>×</span>
                    </button>
                  </div>
                ) : null}
              </div>
            </>
          ) : null}
        </div>
      </nav>
    </>
  );
}