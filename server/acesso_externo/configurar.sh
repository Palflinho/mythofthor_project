#!/bin/bash
# Deixa o servidor acessivel para quem esta na mesma rede da Radmin VPN (teste fechado).
#
# O que faz:
#  1) char_ip / map_ip = o IP da Radmin VPN deste PC: e o endereco que o servidor manda ao cliente depois do login
#     (com 127.0.0.1 o cliente do amigo tentaria conectar no PC dele mesmo)
#  2) Troca a senha padrao entre os servidores (s1/p1) por uma aleatoria: quem chega na porta 6900 nao consegue se
#     registrar como char-server. Fica em ~/mythofthor-server/senha_servidores (fora do repositorio).
#  3) Reinicia char e map.
#
# Uso (WSL):  bash server/acesso_externo/configurar.sh 26.x.x.x     (o IP que a Radmin VPN mostra para este PC)
#             bash server/acesso_externo/configurar.sh 127.0.0.1    (volta a ser so local)
set -e
IP="$1"
if [[ ! "$IP" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Uso: $0 <IP da Radmin VPN deste PC>  (ex.: 26.12.34.56)"; exit 1
fi
RATHENA="${RATHENA_DIR:-$HOME/mythofthor-server/rathena}"
ASSET="$RATHENA/tools/docker/asset"
SECRET_FILE="$HOME/mythofthor-server/senha_servidores"

# 1) IP anunciado
sed -i "s/^char_ip: .*/char_ip: $IP/" "$ASSET/char_conf.txt"
sed -i "s/^map_ip: .*/map_ip: $IP/" "$ASSET/map_conf.txt"

# 2) Senha entre servidores (gerada uma vez)
if [ ! -s "$SECRET_FILE" ]; then
  head -c 32 /dev/urandom | base64 | tr -dc 'A-Za-z0-9' | head -c 20 > "$SECRET_FILE"
  chmod 600 "$SECRET_FILE"
fi
PASS="$(cat "$SECRET_FILE")"
USER_S="mythsrv"
for f in "$ASSET/char_conf.txt" "$ASSET/map_conf.txt"; do
  sed -i '/^userid: /d; /^passwd: /d' "$f"
  printf 'userid: %s\npasswd: %s\n' "$USER_S" "$PASS" >> "$f"
done
docker exec mythofthor-db mariadb -uragnarok -pragnarok ragnarok \
  -e "UPDATE login SET userid='$USER_S', user_pass='$PASS' WHERE account_id=1 AND sex='S';"

# 3) Reinicia
cd "$(dirname "$0")/.."
docker restart mythofthor-char mythofthor-map > /dev/null
echo "Servidor anunciando $IP (char/map) com senha entre servidores trocada. Aguarde ~20 s e confira:"
echo "  docker logs --tail 5 mythofthor-map    (deve terminar com 'Map Server is now online')"
