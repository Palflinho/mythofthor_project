#!/bin/bash
# Extrai e compila mapas do data.grf para a Unreal (WSL). A importacao roda depois no Windows (UnrealEditor-Cmd).
# Uso: tools/build_maps.sh <pasta_destino> <mapa> [<mapa> ...]
#   ex.: tools/build_maps.sh /mnt/e/Dev/Ragnarok_Assets/Prontera prt_church prt_castle prt_fild05
set -e
DEST="$1"; shift
TOOLS="$(cd "$(dirname "$0")" && pwd)"
export MYTH_ASSETS="${MYTH_ASSETS:-/mnt/e/Dev/Ragnarok_Assets}"
for MAP in "$@"; do
  echo "=== $MAP"
  python3 "$TOOLS/extract_map.py" "$MAP" "$DEST/$MAP"
  python3 "$TOOLS/compile_map_to_obj.py" "$DEST/$MAP" | grep -E "compiladas|Luzes|Colisao|gerado"
done
