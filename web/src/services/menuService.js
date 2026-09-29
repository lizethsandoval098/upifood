import { request } from './api';

const normalizar = (texto) =>
  String(texto || '')
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .toLowerCase()
    .trim();

const IMAGEN_GENERICA =
  'data:image/svg+xml;utf8,' +
  encodeURIComponent(
    '<svg xmlns="http://www.w3.org/2000/svg" width="640" height="420"><rect width="100%" height="100%" fill="#f2e2cd"/>' +
      '<text x="50%" y="52%" font-size="120" text-anchor="middle" dominant-baseline="middle">☕</text></svg>'
  );

// Busca en el catálogo visual (fotos y categorías de la web) el producto que
// corresponde a uno de la base de datos: primero por nombre exacto y, si no,
// por nombre parecido ("cafe" ~ "Café del Día").
function buscarVisual(nombreDb, catalogoVisual) {
  const nombre = normalizar(nombreDb);

  const exacto = catalogoVisual.find((item) => normalizar(item.name) === nombre);
  if (exacto) return exacto;

  if (nombre.length < 4) return null;

  return (
    catalogoVisual.find((item) => {
      const visual = normalizar(item.name);
      return visual.includes(nombre) || nombre.includes(visual);
    }) || null
  );
}

// Trae el menú REAL (nombre, precio y stock) de la base de datos y le pone
// foto y categoría con el catálogo visual de la web.
export async function cargarMenu(catalogoVisual = []) {
  const { cafeterias } = await request('/api/cafeterias');

  if (!cafeterias?.length) {
    throw new Error('Todavía no hay cafeterías disponibles.');
  }

  const cafeteria = cafeterias[0];
  const { productos } = await request(`/api/cafeterias/${encodeURIComponent(cafeteria.id)}/productos`);

  return productos.map((producto) => {
    const visual = buscarVisual(producto.nombre, catalogoVisual);

    return {
      id: producto.id, // id real de la base de datos (ej. C1-01): es el que viaja en el pedido
      name: producto.nombre.charAt(0).toUpperCase() + producto.nombre.slice(1),
      price: producto.precio,
      stock: producto.stock,
      category: visual?.category ?? 'Otros',
      description: visual?.description,
      image: visual?.image || IMAGEN_GENERICA,
      cafeteriaId: cafeteria.id,
    };
  });
}
