#!/bin/bash
# Gera db/import/job_stats.yml do Myth of Thor: nivel base maximo 125 para transclasse e classes expandidas,
# com a curva de EXP 99..125 tirada da tabela oficial da Renovacao (grupo das 3as classes em db/re/job_exp.yml).
set -e
cd "${RATHENA_DIR:-/home/palf/mythofthor-server/rathena}"
F=db/import/job_stats.yml
grep -q "Myth of Thor" "$F" && { echo "ja gerado"; exit 0; }
cp "$F" "$F.orig"

EXP=$(sed -n '573,1040p' db/re/job_exp.yml | awk '
  /^      - Level: /{lv=$3}
  /^        Exp: /{ if (lv>=99 && lv<=125) printf "      - Level: %d\n        Exp: %s\n", lv, $2 }')

grep -q "^Header:" "$F" || { echo "sem Header no import"; exit 1; }
grep -q "^Body:" "$F" && { echo "import ja tem Body; revisar a mao"; exit 1; }

cat >> "$F" <<EOF

# --- Myth of Thor (2026-10-12) ---------------------------------------------
# Regras da Renovacao, jogo ate a transclasse. Transclasse 125/70; classes expandidas finais 125 (job padrao);
# 2a classe continua 99/50. Atributos maximos 110 ficam em conf/import/battle_conf.txt.
Body:
  - Jobs:
      Lord_Knight: true
      High_Priest: true
      High_Wizard: true
      Whitesmith: true
      Sniper: true
      Assassin_Cross: true
      Paladin: true
      Champion: true
      Professor: true
      Stalker: true
      Creator: true
      Clown: true
      Gypsy: true
      Super_Novice: true
      Gunslinger: true
      Ninja: true
      Star_Gladiator: true
      Soul_Linker: true
    MaxBaseLevel: 125
    BaseExp:
$EXP
EOF
echo "ok"; tail -70 "$F" | head -40
