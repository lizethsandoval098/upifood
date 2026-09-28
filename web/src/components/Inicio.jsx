import heroImage from '../assets/upiifood.png';

export default function Inicio() {
  return (
    <>
      <style>{`
        .home-view {
          min-height: calc(100vh - 86px);
          display: grid;
          align-items: center;
          padding: clamp(42px, 7vw, 88px) clamp(24px, 7vw, 96px);
          background:
            var(--upifood-floral-pattern) left top / 300px repeat,
            var(--upifood-cream-gradient);
          overflow: hidden;
        }

        .home-layout {
          width: min(1240px, 100%);
          margin: 0 auto;
          display: grid;
          grid-template-columns: minmax(0, 0.95fr) minmax(360px, 1.05fr);
          align-items: center;
          gap: clamp(32px, 6vw, 76px);
        }

        .home-copy {
          max-width: 610px;
        }

        .home-eyebrow {
          margin: 0 0 18px;
          color: #8a5a2b;
          font-size: 0.78rem;
          font-weight: 800;
          letter-spacing: 2px;
          text-transform: uppercase;
        }

        .home-title {
          margin: 0;
          color: #3c2a21;
          font-family: 'Playfair Display', Georgia, serif;
          font-size: clamp(2.8rem, 5.4vw, 5.3rem);
          font-weight: 700;
          line-height: 1.02;
          max-width: 680px;
        }

        .home-title span {
          color: #95633a;
        }

        .home-description {
          max-width: 580px;
          margin: 24px 0 30px;
          color: #5c4636;
          font-size: clamp(1rem, 1.3vw, 1.15rem);
          line-height: 1.75;
        }

        .home-cta {
          display: inline-flex;
          align-items: center;
          gap: 18px;
          min-height: 52px;
          padding: 0 22px;
          border-radius: 12px;
          background: #3c2a21;
          color: #fff8ef;
          font-weight: 700;
          text-decoration: none;
          box-shadow: 0 10px 24px rgba(60, 42, 33, 0.18);
          transition: transform 0.18s ease, background 0.18s ease;
        }

        .home-cta:hover {
          background: #5a3d2b;
          transform: translateY(-2px);
        }

        .home-cta-arrow {
          font-size: 1.2rem;
          line-height: 1;
        }

        .home-image-wrap {
          position: relative;
          width: min(120%, 660px);
          aspect-ratio: 1;
          justify-self: center;
          border-radius: 50%;
          overflow: visible;
        }

        .home-image-wrap::before {
          content: '';
          position: absolute;
          inset: 0;
          border-radius: 50%;
          background: #d4b18a;
          box-shadow: 0 24px 54px rgba(60, 42, 33, 0.2);
        }

        .home-image {
          position: absolute;
          top: 50%;
          left: 50%;
          z-index: 1;
          width: 120%;
          height: 120%;
          object-fit: contain;
          object-position: center;
          opacity: 0.88;
          transform: translate(-50%, -50%);
        }

        .home-image-caption {
          position: absolute;
          right: 18px;
          bottom: 18px;
          max-width: calc(100% - 36px);
          padding: 11px 15px;
          border: 1px solid rgba(255, 248, 239, 0.45);
          border-radius: 6px;
          background: rgba(47, 32, 24, 0.78);
          color: #fff8ef;
          font-size: 0.9rem;
          font-weight: 600;
          backdrop-filter: blur(6px);
        }

        @media (max-width: 820px) {
          .home-view {
            min-height: auto;
            padding-block: 48px;
          }

          .home-layout {
            grid-template-columns: 1fr;
            gap: 36px;
          }

          .home-image-wrap {
            width: min(100%, 520px);
          }
        }

        @media (max-width: 480px) {
          .home-image-wrap {
            width: min(100%, 360px);
          }
        }
      `}</style>

      <section className="home-view" id="inicio" aria-labelledby="home-title">
        <div className="home-layout">
          <div className="home-copy">
            <p className="home-eyebrow">Upiifood · Cafetería estudiantil</p>
            <h1 className="home-title" id="home-title">
              Pide rápido,<br />
              come rico y<br />
              <span>sigue tu día.</span>
            </h1>
            <p className="home-description">
              En Upiifood trabajamos para satisfacer a la comunidad estudiantil con comida rica,
              un servicio ágil y una experiencia eficiente que acompaña tu rutina en el Poli.
            </p>
            <a className="home-cta" href="#menu">
              Explorar menú
              <span className="home-cta-arrow" aria-hidden="true">→</span>
            </a>
          </div>

          <div className="home-image-wrap" aria-label="Selección de cafetería Upiifood">
            <img className="home-image" src={heroImage} alt="Ilustración de la cafetería Upiifood" />
          </div>
        </div>
      </section>
    </>
  );
}
