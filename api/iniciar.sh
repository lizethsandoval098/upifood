#!/bin/bash
# Arranca la API web (puente entre la página y el servidor C++).
cd "$(dirname "$0")" || exit 1
[ -f .env ] || { cp .env.example .env; echo "[+] Se creó api/.env: edítalo (sobre todo WEB_ORIGIN) y vuelve a ejecutar."; exit 0; }
[ -d node_modules ] || npm install || exit 1
exec node server.js
