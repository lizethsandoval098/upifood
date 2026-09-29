#!/bin/bash
# UPIIFOOD - compilar y ejecutar
# La base de datos vive SIEMPRE en database/upifood.db (ya no depende de la
# carpeta desde donde ejecutes el servidor), asi que ya no hace falta
# respaldarla ni restaurarla aqui.

cd "$(dirname "$0")" || exit 1

# Si la carpeta build/ vino de OTRA computadora, su CMakeCache.txt apunta a
# rutas que no existen aqui y cmake falla. En ese caso se borra sola.
if [ -f build/CMakeCache.txt ]; then
    RUTA_CACHE=$(grep -m1 "^CMAKE_HOME_DIRECTORY" build/CMakeCache.txt | cut -d= -f2)
    if [ "$RUTA_CACHE" != "$(pwd)" ]; then
        echo "[+] build/ viene de otra computadora, se limpia..."
        rm -rf build
    fi
fi

mkdir -p build database
cd build || exit 1

echo "[+] Configurando y compilando con CMake..."
cmake .. || { echo "[-] Fallo cmake (revisa que tengas SFML y sqlite3 instalados)."; exit 1; }
make -j"$(nproc 2>/dev/null || echo 2)" || { echo "[-] Fallo la compilacion."; exit 1; }

echo ""
echo "=========================================="
echo "   ¿Que ejecutable deseas arrancar?"
echo "=========================================="
echo "1) Servidor      (./servidor)"
echo "2) Cliente       (./cliente)"
echo "3) Cafeteria     (./cafeteria)"
echo "4) Administrador (./administrador)"
echo "5) Solo compilar (Salir)"
echo "=========================================="
read -p "Ingresa una opcion [1-5]: " opcion

# Para cliente/cafeteria/administrador puedes pasar la IP del servidor:
#   ./ejecutar.sh 100.70.231.3   (o exporta UPIIFOOD_IP)
IP="$1"

case $opcion in
    1) ./servidor ;;
    2) ./cliente $IP ;;
    3) ./cafeteria $IP ;;
    4) ./administrador $IP ;;
    5) echo "[+] Compilacion lista."; exit 0 ;;
    *) echo "[-] Opcion no valida."; exit 1 ;;
esac
