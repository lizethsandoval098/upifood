import { request, setToken } from './api';

// Crea la cuenta en la base de datos (tabla Usuarios/Clientes del servidor C++).
export async function registrarCuenta({ nombreCompleto, correo, password, escuela }) {
  const data = await request('/api/auth/registro', {
    method: 'POST',
    body: JSON.stringify({ nombreCompleto, correo, password, escuela }),
  });

  setToken(data.token);
  return data.usuario;
}

export async function iniciarSesion({ correo, password }) {
  const data = await request('/api/auth/login', {
    method: 'POST',
    body: JSON.stringify({ correo, password }),
  });

  setToken(data.token);
  return data.usuario;
}

// Los invitados piden con una cuenta compartida (no tienen historial).
export async function entrarComoInvitado() {
  const data = await request('/api/auth/invitado', { method: 'POST' });
  setToken(data.token);
  return data.usuario;
}

export const cerrarSesion = () => setToken(null);
