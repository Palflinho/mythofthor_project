#!/bin/bash
# Myth of Thor: regras da Renovacao com o jogo ate a transclasse (decisao do usuario, 2026-10-12).
# Aplica no rAthena do WSL (rodar como root: os arquivos de import pertencem ao root do container):
#   wsl.exe -d Ubuntu -u root -- bash "/mnt/e/Documentos/Repositórios/mythofthor_project/server/renovacao/aplicar.sh"
# Pode rodar de novo sem duplicar nada. Depois recompile (ver docs/PRERE_X_RENOVACAO.md, seção "Como foi aplicado").
set -e
RA=${RATHENA_DIR:-/home/palf/mythofthor-server/rathena}
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$RA"

# 1. Atributos maximos 110 (2a classe, transclasse e expandidas; bebes ficam no padrao 80)
cat > conf/import/battle_conf.txt <<'EOF'
// Myth of Thor (2026-10-12): atributos maximos 110 ate a transclasse
max_parameter: 110
max_trans_parameter: 110
EOF

# 2. Nivel base 125 para transclasse e classes expandidas finais (curva oficial da Renovacao)
bash "$HERE/gen_job_stats.sh"

# 3. Sem criacao de Invocador (Doram): so Aprendiz. O char_conf do import vem do asset do docker.
ASSET=tools/docker/asset/char_conf.txt
grep -q "^allowed_job_flag" "$ASSET" || printf '\n// Myth of Thor: so Aprendiz (sem Invocador/Doram)\nallowed_job_flag: 1\n' >> "$ASSET"

# 4. Sem mudanca para classes depois da transclasse (3as classes, Kagerou/Oboro, Rebelde, Super Aprendiz Expandido, Doram)
mkdir -p npc/custom/myth
cp "$HERE/escopo_classes.conf" npc/custom/myth/escopo_classes.conf
grep -q "escopo_classes.conf" npc/scripts_custom.conf || echo "import: npc/custom/myth/escopo_classes.conf" >> npc/scripts_custom.conf

# 5. Mensagens do servidor em portugues: liga o pacote de idioma (precisa recompilar), converte o arquivo para UTF-8
#    (o cliente le o texto do servidor como UTF-8) e poe cada conta em portugues ao entrar
DEF=src/custom/defines_pre.hpp
grep -q "LANG_ENABLE" "$DEF" || sed -i 's|^#endif /\* CONFIG_CUSTOM_DEFINES_PRE_HPP \*/|// Myth of Thor: pacote de idioma portugues (LANG_POR)\n#define LANG_ENABLE 0x80\n\n#endif /* CONFIG_CUSTOM_DEFINES_PRE_HPP */|' "$DEF"
grep -q "LANG_ENABLE" "$DEF" || { echo "nao consegui pôr LANG_ENABLE em $DEF"; exit 1; }
MSG=conf/msg_conf/map_msg_por.conf
file "$MSG" | grep -q "UTF-8" || { iconv -f ISO-8859-1 -t UTF-8 "$MSG" > /tmp/map_msg_por.conf && cp /tmp/map_msg_por.conf "$MSG"; }
cp "$HERE/idioma.txt" npc/custom/myth/idioma.txt
grep -q "myth/idioma.txt" npc/scripts_custom.conf || echo "npc: npc/custom/myth/idioma.txt" >> npc/scripts_custom.conf
cp "$HERE/../npc/custom/myth/mvp_comandos.txt" npc/custom/myth/mvp_comandos.txt
grep -q "myth/mvp_comandos.txt" npc/scripts_custom.conf || echo "npc: npc/custom/myth/mvp_comandos.txt" >> npc/scripts_custom.conf
cp "$HERE/../npc/custom/myth/treinadora_classes.txt" npc/custom/myth/treinadora_classes.txt
grep -q "myth/treinadora_classes.txt" npc/scripts_custom.conf || echo "npc: npc/custom/myth/treinadora_classes.txt" >> npc/scripts_custom.conf

# 6. Compilacao em modo Renovacao (PRERE desligado) com o mesmo PACKETVER
echo "Pronto. Recompile com:"
echo "  cd /mnt/e/Documentos/Repositórios/mythofthor_project/server && docker compose --profile build run --rm builder sh -c './configure --enable-packetver=20250402 --enable-prere=no && make clean server'"
