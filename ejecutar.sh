#!/bin/bash

# 1. Copia de respaldo de la BD si existe dentro de build
if [ -f "build/upifood.db" ]; then
    cp build/upifood.db ./upifood_backup.db
    echo "[+] Respaldo de upifood.db creado."
fi

# 2. Preparar carpeta build y limpiar caché previa
mkdir -p build
cd build
rm -rf CMakeCache.txt CMakeFiles/

# 3. Configurar con CMake y compilar con Make
echo "[+] Configurando y compilando con CMake..."
cmake ..
make

# 4. Restaurar la base de datos previa
if [ -f "../upifood_backup.db" ]; then
    cp ../upifood_backup.db ./upifood.db
    echo "[+] Base de datos upifood.db restaurada con exito."
fi

# 5. Menú interactivo de selección
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

case $opcion in
    1) ./servidor ;;
    2) ./cliente ;;
    3) ./cafeteria ;;
    4) ./administrador ;;
    5) echo "[+] Compilacion lista."; exit 0 ;;
    *) echo "[-] Opcion no valida."; exit 1 ;;
esac
