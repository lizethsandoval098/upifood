// Punto único de comunicación con la API (api/server.js).
// La URL sale de la variable VITE_API_URL:
//   - en tu compu (npm run dev): si no la defines, se usa el proxy de Vite (/api -> localhost:3000)
//   - en Vercel: pon VITE_API_URL=https://TU-MAQUINA.TU-TAILNET.ts.net  (la URL de Tailscale Funnel)
export const API_BASE_URL = (import.meta.env.VITE_API_URL || '').replace(/\/$/, '');

const TOKEN_KEY = 'upifood_token';

export const getToken = () => {
  try {
    return localStorage.getItem(TOKEN_KEY);
  } catch {
    return null;
  }
};

export const setToken = (token) => {
  try {
    if (token) localStorage.setItem(TOKEN_KEY, token);
    else localStorage.removeItem(TOKEN_KEY);
  } catch {
    // Sin almacenamiento local la sesión solo dura mientras la pestaña siga abierta.
  }
};

export async function request(path, options = {}) {
  const token = getToken();
  let response;

  try {
    response = await fetch(`${API_BASE_URL}${path}`, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...(token ? { Authorization: `Bearer ${token}` } : {}),
        ...(options.headers || {}),
      },
    });
  } catch {
    throw new Error('No se pudo conectar con la cafetería. Revisa tu conexión e inténtalo de nuevo.');
  }

  const isJson = (response.headers.get('content-type') || '').includes('application/json');
  const body = isJson ? await response.json() : await response.text();

  if (!response.ok) {
    const message =
      typeof body === 'string' ? body : body?.error || body?.message || `La petición falló (${response.status}).`;
    const error = new Error(message);
    error.status = response.status;
    throw error;
  }

  return body;
}
