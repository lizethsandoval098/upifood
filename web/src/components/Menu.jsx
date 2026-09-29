import { useCallback, useEffect, useMemo, useState } from 'react';
import { cargarMenu } from '../services/menuService';

// Fotos y categorías de la web. El menú real (nombre, precio y stock) sale de la
// base de datos; esta lista solo le pone la foto/categoría si el nombre coincide.
const catalogoVisual = [
  {
    id: 1,
    name: 'Latte Escolar',
    category: 'Bebida Caliente',
    //description: 'Espresso doble con leche cremosa, ideal para la primera clase.',
    price: 58,
    image:
      'https://img.postershop.me/cdn-cgi/image/width=1024,format=webp/https://img.postershop.me/20247/ed903be3-0b22-4a4c-9034-01be719d6a33_image.jpeg',
  },
  {
    id: 2,
    name: 'Capuchino Canela',
    category: 'Bebida Fría',
    //description: 'Espuma suave con toque de canela y cacao.',
    price: 62,
    image:
      'https://images.unsplash.com/photo-1461023058943-07fcbe16d735?auto=format&fit=crop&w=900&q=80',
  },
  {
    id: 3,
    name: 'Cold Brew UPII',
    category: 'Bebida Fría',
    //description: 'Extraccion lenta de 12h, fresco y con energia sostenida.',
    price: 65,
    image:
      'https://images.unsplash.com/photo-1517701604599-bb29b565090c?auto=format&fit=crop&w=900&q=80',
  },
  {
    id: 4,
    name: 'Frappuccino Moka',
    category: 'Bebida Fría',
    //description: 'Cafe helado con cacao, crema batida y chips de chocolate.',
    price: 60,
    image:
      'https://lafoy.ru/photo_l/foto-3674-5.jpg',
  },
  {
    id: 5,
    name: 'Mollete Integral',
    category: 'Desayuno',
    //description: 'Pan integral con frijol, queso gratinado y pico de gallo.',
    price: 40,
    image:
      'https://images.unsplash.com/photo-1484723091739-30a097e8f929?auto=format&fit=crop&w=900&q=80',
  },
  {
    id: 6,
    name: 'Hojaldra de Mole',
    category: 'Panaderia',
    //description: 'Pollo, panela, jitomate y aderezo de yogur en pan de caja.',
    price: 30,
    image:
      'https://th.bing.com/th/id/R.fb501441984368538180d59d1eb59a57?rik=wG38mJ0nt40nvQ&riu=http%3a%2f%2fcdn2.cocinadelirante.com%2fsites%2fdefault%2ffiles%2fimages%2f2017%2f07%2fhojaldrasdemoleconpollo.jpg&ehk=AdyoewtivbAEsfSl5EgV4%2fXPxasVGsEmYSPaBJ87qbQ%3d&risl=&pid=ImgRaw&r=0',
  },
  {
    id: 7,
    name: 'Panque de Platano',
    category: 'Panaderia',
    //description: 'Rebanada casera con nuez, perfecta para el receso.',
    price: 38,
    image:
      'https://images.unsplash.com/photo-1606313564200-e75d5e30476c?auto=format&fit=crop&w=900&q=80',
  },
  {
    id: 8,
    name: 'Cuernito',
    category: 'Panaderia',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 25,
    image:
      'https://campra.com.mx/wp-content/uploads/2025/10/cuernitos-jamon-queso.jpg',
  },
  {
    id: 9,
    name: 'Club sandwich',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 45,
    image:
      'https://okdiario.com/img/2021/07/30/sandwich-club.jpg',
  },
  {
    id: 10,
    name: 'Enchiladas Suizas',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 60,
    image:
      'https://dirtydishesmessykisses.com/wp-content/uploads/2024/11/enchiladas-suizas-recipe-1732016780.jpg',
  },

  {
    id: 11,
    name: 'Chocolate Caliente',
    category: 'Bebida Caliente',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 20,
    image:
      'https://elrinconcolombiano.com/wp-content/uploads/2024/05/Chocolate-Caliente-receta-colombiana.jpg',
  },

  {
    id: 12,
    name: 'Chilaquiles',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 50,
    image:
      'https://assets.tmecosys.com/image/upload/t_web_rdp_recipe_584x480_1_5x/img/recipe/ras/Assets/7bff10bca40f84f0d1e1bfc19734096e/Derivates/a28c3a0209934a6797425ccc6383f469524f24bf.jpg',
  },

  {
    id: 13,
    name: 'Frappuccino de Oreo',
    category: 'Bebida Fría',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 60,
    image:
      'https://deliciosareceta.com/wp-content/uploads/2025/03/Image_2-93.png',
  },

   {
    id: 14,
    name: 'Chapata de Atún',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 60,
    image:
      'https://sabordemex.mx/wp-content/uploads/2024/09/chapata-de-atun-1.png',
  },

  {
    id: 15,
    name: 'Hamburguesa',
    category: 'Comida',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 45,
    image:
      'https://resizer.glanacion.com/resizer/v2/hamburguesa-FHBQ5XJM55H2PFSAFSC6HHESVQ.jpg?auth=c14fd6c0f7fd21e554cb59b5d69f7ee3551b78c00f500eb190a79ae39dc0ae80&width=768&height=512&quality=70&smart=true',
  },

  {
    id: 16,
    name: 'Torta de Asada',
    category: 'Comida',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 45,
    image:
      'https://www.maricruzavalos.com/wp-content/uploads/2023/08/carne-asada-torta-recipe.jpg',
  },

  {
    id: 17,
    name: 'Hot Dogs',
    category: 'Comida',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 55,
    image:
      'https://media.istockphoto.com/id/1166632799/es/foto/perro-chile-casero-estilo-detroit-en-una-superficie-negra-vista-superior-estaba-plano-desde.jpg?s=612x612&w=0&k=20&c=ljhVn401SwraenXjo5BF7MtgZuDP0P6tjvPq4BFdvVY=',
  },

  {
    id: 18,
    name: 'Café del Día',
    category: 'Bebida Caliente',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 20,
    image:
      'https://img.freepik.com/fotos-premium/taza-steamy-elixir-cafe-caliente-espuma-delicada_1008992-2087.jpg',
  },
  
  {
    id: 19,
    name: 'Agua de Jamaica',
    category: 'Bebida Fría',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 20,
    image:
      'https://marialauragarcia.com/wp-content/uploads/2023/09/jugo-roselle-vaso-listo-beber.webp',
  },
  
  {
    id: 20,
    name: 'Agua de Horchata',
    category: 'Bebida Fría',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 20,
    image:
      'https://comidamexicana.online/wp-content/uploads/2020/01/receta-agua-de-horchata-de-avena-1024x683.jpg',
  },

  {
    id: 21,
    name: 'Té Caliente',
    category: 'Bebida Caliente',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 25,
    image:
      'https://img.freepik.com/fotos-premium/te-caliente-taza-sobre-superficie-vieja_231794-2782.jpg',
  },
  
  {
    id: 22,
    name: 'Hot cakes',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 30,
    image:
      'https://es.chefyoyo.com/thumb/1024/los-hot-cakes-perfectos-como-los-de-la-abuela.webp',
  },

  {
    id: 23,
    name: 'Omelette',
    category: 'Desayuno',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 40,
    image:
      'https://www.healthyfood.com/wp-content/uploads/2018/02/Basic-omelette.jpg',
  },

  {
    id: 24,
    name: 'Burrito',
    category: 'Comida',
    //description: 'Latte + sandwich + fruta del dia con precio especial.',
    price: 65,
    image:
      'https://epicureannest.com/wp-content/uploads/2025/02/0_2_640_N-134-e1739901615518.webp',
  },
];

const SEGUNDOS_ACTUALIZACION = 20;

export default function Menu({ onAddToCart = () => {} }) {
  const [lastAdded, setLastAdded] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('Todo');
  const [quantities, setQuantities] = useState({});
  const [menuItems, setMenuItems] = useState([]);
  const [menuStatus, setMenuStatus] = useState('cargando'); // cargando | listo | error
  const [menuError, setMenuError] = useState('');

  // Trae el menú de la base de datos y lo refresca cada 20 s (por si la cafetería cambia precios o stock).
  const loadMenu = useCallback(async () => {
    try {
      const items = await cargarMenu(catalogoVisual);
      setMenuItems(items);
      setMenuStatus('listo');
      setMenuError('');
    } catch (error) {
      setMenuError(error.message);
      setMenuStatus((estado) => (estado === 'listo' ? 'listo' : 'error'));
    }
  }, []);

  useEffect(() => {
    loadMenu();
    const intervalId = window.setInterval(loadMenu, SEGUNDOS_ACTUALIZACION * 1000);
    return () => window.clearInterval(intervalId);
  }, [loadMenu]);

  const getQty = (id) => quantities[id] ?? 1;

  const normalizedMenuItems = useMemo(
    () => menuItems.map((item) => ({ ...item, categoria: item.categoria ?? item.category })),
    [menuItems]
  );

  const categories = useMemo(
    () => ['Todo', ...new Set(normalizedMenuItems.map((item) => item.categoria))],
    [normalizedMenuItems]
  );

  const filteredItems = useMemo(
    () =>
      selectedCategory === 'Todo'
        ? normalizedMenuItems
        : normalizedMenuItems.filter((item) => item.categoria === selectedCategory),
    [normalizedMenuItems, selectedCategory]
  );

  const updateQty = (item, delta) => {
    const maximo = Math.max(1, Math.min(20, item.stock ?? 20));

    setQuantities((prev) => ({
      ...prev,
      [item.id]: Math.max(1, Math.min(maximo, (prev[item.id] ?? 1) + delta)),
    }));
  };

  const addToCart = (item) => {
    const qty = getQty(item.id);
    onAddToCart(item, qty);
    setLastAdded(`${qty} x ${item.name}`);
  };

  return (
    <>
      <style>{`
        .menu-section {
          position: relative;
          padding: 72px 22px 84px;
          background:
            var(--upifood-floral-pattern) left top / 300px repeat,
            radial-gradient(circle at 18% 0%, rgba(250, 236, 219, 0.92) 0%, rgba(250, 236, 219, 0) 42%),
            linear-gradient(180deg, #f8eedf 0%, #f2e2cd 55%, #ecd6bc 100%);
          overflow: hidden;
        }

        .menu-wrap {
          max-width: 1180px;
          margin: 0 auto;
        }

        .menu-head {
          display: flex;
          justify-content: space-between;
          align-items: flex-end;
          gap: 18px;
          margin-bottom: 22px;
        }

        .menu-subtitle {
          margin: 0 0 8px;
          color: #8a5a2b;
          font-size: 13px;
          font-weight: 700;
          letter-spacing: 2.2px;
          text-transform: uppercase;
        }

        .menu-title {
          margin: 0;
          color: #3d2717;
          font-family: 'Playfair Display', serif;
          font-size: clamp(1.9rem, 3.4vw, 2.8rem);
          line-height: 1.1;
        }

        .cart-pill {
          background: #3d2717;
          color: #fff5e8;
          border-radius: 999px;
          font-weight: 700;
          font-size: 0.95rem;
          padding: 10px 16px;
          box-shadow: 0 8px 22px rgba(61, 39, 23, 0.24);
          white-space: nowrap;
        }

        .menu-categories {
          display: flex;
          gap: 10px;
          flex-wrap: wrap;
          margin-bottom: 26px;
        }

        .category-chip {
          border: 1px solid #d4b691;
          background: rgba(255, 248, 239, 0.85);
          color: #5c3a24;
          border-radius: 999px;
          font-size: 0.9rem;
          font-weight: 600;
          padding: 8px 13px;
          cursor: pointer;
          transition: all 0.18s ease;
        }

        .category-chip--active {
          background: #4a2e1b;
          border-color: #4a2e1b;
          color: #fffaf3;
          box-shadow: 0 8px 18px rgba(74, 46, 27, 0.2);
        }

        .menu-grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(245px, 1fr));
          gap: 18px;
        }

        .menu-card {
          display: flex;
          flex-direction: column;
          border-radius: 20px;
          overflow: hidden;
          background: #fffdf9;
          box-shadow: 0 12px 28px rgba(105, 68, 42, 0.15);
          border: 1px solid #efd9bd;
          transition: transform 0.18s ease, box-shadow 0.18s ease;
        }

        .menu-card:hover {
          transform: translateY(-4px);
          box-shadow: 0 18px 30px rgba(105, 68, 42, 0.2);
        }

        .menu-image {
          width: 100%;
          height: 172px;
          object-fit: cover;
        }

        .menu-body {
          padding: 14px 14px 16px;
          display: flex;
          flex-direction: column;
          gap: 10px;
        }

        .menu-meta {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 10px;
        }

        .menu-item-name {
          margin: 0;
          color: #3f2816;
          font-size: 1.08rem;
          line-height: 1.2;
          font-weight: 700;
        }

        .menu-price {
          color: #9d4f1e;
          font-weight: 800;
          font-size: 1.02rem;
          background: #fff1e3;
          border-radius: 10px;
          padding: 6px 9px;
          border: 1px solid #f1cfac;
        }

        .menu-item-description {
          margin: 0;
          color: #6d4f3b;
          font-size: 0.92rem;
          line-height: 1.45;
          min-height: 54px;
        }

        .card-footer {
          display: flex;
          align-items: center;
          gap: 10px;
        }

        .qty-control {
          display: inline-flex;
          align-items: center;
          border: 1px solid #e2c5a2;
          border-radius: 10px;
          overflow: hidden;
          background: #fff7ee;
          flex-shrink: 0;
        }

        .qty-btn {
          border: none;
          width: 32px;
          height: 36px;
          color: #6a4328;
          background: transparent;
          font-size: 1rem;
          font-weight: 700;
          cursor: pointer;
        }

        .qty-value {
          min-width: 28px;
          text-align: center;
          color: #4b301e;
          font-weight: 700;
          font-size: 0.95rem;
        }

        .add-cart-btn {
          border: none;
          border-radius: 11px;
          background: linear-gradient(130deg, #7a4a2a 0%, #a55c2e 100%);
          color: #fff;
          font-weight: 700;
          font-size: 0.9rem;
          height: 36px;
          padding: 0 12px;
          cursor: pointer;
          flex: 1;
          box-shadow: 0 7px 16px rgba(122, 74, 42, 0.3);
          transition: filter 0.18s ease;
        }

        .add-cart-btn:hover {
          filter: brightness(1.06);
        }

        .menu-note {
          margin: 20px 2px 0;
          color: #76543d;
          font-weight: 600;
          font-size: 0.92rem;
        }

        .menu-empty {
          grid-column: 1 / -1;
          padding: 30px 24px;
          border-radius: 18px;
          background: rgba(255, 248, 239, 0.88);
          border: 1px dashed #d9b68c;
          color: #6d4f3b;
          text-align: center;
          font-size: 0.98rem;
          line-height: 1.5;
        }

        @media (max-width: 780px) {
          .menu-head {
            flex-direction: column;
            align-items: flex-start;
          }

          .cart-pill {
            font-size: 0.88rem;
            padding: 9px 14px;
          }

          .menu-grid {
            grid-template-columns: repeat(auto-fill, minmax(210px, 1fr));
            gap: 14px;
          }
        }
      `}</style>

      <section id="menu" className="menu-section">
        <div className="menu-wrap">
          <div className="menu-head">
            <div>
              <p className="menu-subtitle">Menu escolar completo</p>
              <h2 className="menu-title">Pide rapido, come rico y sigue tu dia</h2>
            </div>
            <div className="cart-pill">{lastAdded ? `Ultimo agregado: ${lastAdded}` : 'Aun no agregas productos'}</div>
          </div>

          <div className="menu-categories">
            {categories.map((category) => (
              <button
                key={category}
                type="button"
                className={`category-chip ${selectedCategory === category ? 'category-chip--active' : ''}`}
                onClick={() => setSelectedCategory(category)}
              >
                {category}
              </button>
            ))}
          </div>

          <div className="menu-grid">
            {filteredItems.length > 0 ? (
              filteredItems.map((item) => (
                <article className="menu-card" key={item.id}>
                  <img src={item.image} alt={item.name} className="menu-image" />
                  <div className="menu-body">
                    <div className="menu-meta">
                      <h3 className="menu-item-name">{item.name}</h3>
                      <span className="menu-price">${item.price}</span>
                    </div>
                    <p className="menu-item-description">{item.description ?? 'Delicioso producto listo para disfrutar.'}</p>

                    <div className="card-footer">
                      <div className="qty-control" aria-label={`Cantidad de ${item.name}`}>
                        <button
                          className="qty-btn"
                          onClick={() => updateQty(item, -1)}
                          aria-label={`Quitar una unidad de ${item.name}`}
                        >
                          -
                        </button>
                        <span className="qty-value">{getQty(item.id)}</span>
                        <button
                          className="qty-btn"
                          onClick={() => updateQty(item, 1)}
                          aria-label={`Agregar una unidad de ${item.name}`}
                        >
                          +
                        </button>
                      </div>

                      <button
                        className="add-cart-btn"
                        onClick={() => addToCart(item)}
                        disabled={item.stock <= 0}
                        style={item.stock <= 0 ? { opacity: 0.55, cursor: 'not-allowed' } : undefined}
                      >
                        {item.stock <= 0 ? 'Agotado' : 'Agregar al carrito'}
                      </button>
                    </div>
                  </div>
                </article>
              ))
            ) : (
              <div className="menu-empty">
                {menuStatus === 'cargando'
                  ? 'Cargando el menú...'
                  : menuStatus === 'error'
                    ? `No se pudo cargar el menú. ${menuError}`
                    : 'No hay productos disponibles en esta categoría por el momento.'}
                {menuStatus === 'error' ? (
                  <button type="button" className="category-chip" style={{ marginLeft: 12 }} onClick={loadMenu}>
                    Reintentar
                  </button>
                ) : null}
              </div>
            )}
          </div>

          {lastAdded ? <p className="menu-note">Ultimo agregado: {lastAdded}</p> : null}
        </div>
      </section>
    </>
  );
}