const profileFields = [
  ['Nombre completo', 'nombreCompleto'],
  ['Correo institucional', 'correo'],
  ['Año de ingreso al Poli', 'anioIngreso'],
  ['Escuela de procedencia', 'escuela'],
];

const schoolNames = {
  upiita: 'UPIITA',
  esime: 'ESIME',
  esca: 'ESCA',
  esfm: 'ESFM',
  encb: 'ENCB',
  upiih: 'UPIIH',
  cecyt: 'CECyT 9',
  otra: 'Otra',
};

export default function ConfiguracionModal({ open, userData, onClose }) {
  if (!open) return null;

  const profile = {
    nombreCompleto: userData?.nombreCompleto || 'No hay nombre registrado',
    correo: userData?.correo || 'No hay correo registrado',
    anioIngreso: userData?.anioIngreso || 'No especificado',
    escuela: schoolNames[userData?.escuela] || userData?.escuela || 'No especificada',
  };

  return (
    <div
      role="presentation"
      onClick={onClose}
      style={{
        position: 'fixed',
        inset: 0,
        zIndex: 2100,
        display: 'grid',
        placeItems: 'center',
        padding: '20px',
        background: 'rgba(44, 29, 18, 0.48)',
        backdropFilter: 'blur(5px)',
      }}
    >
      <section
        role="dialog"
        aria-modal="true"
        aria-labelledby="settings-title"
        onClick={(event) => event.stopPropagation()}
        style={{
          width: 'min(560px, 100%)',
          overflow: 'hidden',
          border: '1px solid rgba(93, 67, 49, 0.14)',
          borderRadius: '24px',
          background: '#FDF8F2',
          boxShadow: '0 22px 52px rgba(60, 42, 33, 0.28)',
        }}
      >
        <header style={{ padding: '24px 26px 20px', background: 'linear-gradient(180deg, #fdf7f0 0%, #f2e5d6 100%)', borderBottom: '1px solid rgba(93, 67, 49, 0.1)' }}>
          <p style={{ margin: '0 0 8px', color: '#8a5a2b', fontSize: '0.78rem', fontWeight: 700, letterSpacing: '2px', textTransform: 'uppercase' }}>
            Cuenta
          </p>
          <h2 id="settings-title" style={{ margin: 0, color: '#3c2a21', fontFamily: 'Playfair Display, Georgia, serif', fontSize: '2rem' }}>
            Configuración
          </h2>
          <p style={{ margin: '8px 0 0', color: '#6a5241', lineHeight: 1.5 }}>
            Datos del perfil registrado.
          </p>
        </header>

        <div style={{ display: 'grid', gap: '12px', padding: '22px 26px' }}>
          {profileFields.map(([label, key]) => (
            <div key={key} style={{ padding: '13px 15px', border: '1px solid #ead8c2', borderRadius: '12px', background: '#fffdfb' }}>
              <div style={{ marginBottom: '5px', color: '#8a5a2b', fontSize: '0.72rem', fontWeight: 700, letterSpacing: '0.9px', textTransform: 'uppercase' }}>
                {label}
              </div>
              <div style={{ color: '#3c2a21', fontSize: '1rem', fontWeight: 600, overflowWrap: 'anywhere' }}>
                {profile[key]}
              </div>
            </div>
          ))}
        </div>

        <footer style={{ display: 'flex', justifyContent: 'flex-end', padding: '0 26px 24px' }}>
          <button
            type="button"
            onClick={onClose}
            style={{ border: 'none', borderRadius: '12px', padding: '12px 20px', background: '#3D2723', color: '#fff8ef', fontWeight: 700, cursor: 'pointer', boxShadow: '0 8px 18px rgba(60, 42, 33, 0.16)' }}
          >
            Cerrar
          </button>
        </footer>
      </section>
    </div>
  );
}
