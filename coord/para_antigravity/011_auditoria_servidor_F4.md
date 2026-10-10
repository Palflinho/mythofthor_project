status: pendente

# 011: auditoria do servidor, o rAthena também precisa ser 1:1 (prioridade média)

**De:** Claude · **Data:** 2026-10-10 · **Decisão do usuário:** primeiro o jogo fica 1:1 com o RO que já existe.

Contexto: `docs/ROADMAP.md` (fase F4) e `docs/FIDELIDADE.md` (seção "O servidor também precisa ser 1:1").

## O que conferir

1. **Versão e build:** `PACKETVER` em uso (20250402?) e flags (`-DPRERE`). Os docs antigos citam 20211103; diga qual é a
   verdade e liste o que ainda está desatualizado nos docs.
2. **Mudanças em relação ao rAthena pre-renewal puro:** `git diff` do servidor contra o upstream (ou lista dos arquivos
   alterados em `conf/`, `db/pre-re/`, `npc/`, `src/`). Para cada mudança: o quê, por quê, e se é do Myth ou desvio do RO.
3. **Rates e conf:** `conf/battle/*.conf`, `conf/*_athena.conf` comparados com um servidor oficial de referência (rates 1x
   do pre-renewal clássico). Liste os valores diferentes.
4. **Bancos:** `mob_db` (HP, ATK, drops, EXP), `item_db`, `skill_db` (custo de SP, cast, delay, alcance, dano) do
   `pre-re` contra o oficial. Amostragem mínima: monstros de Prontera e Izlude, itens de uso e skills da 1ª classe.
5. **Spawns e NPCs:** `npc/pre-re` dos mapas importados (coord 013) conferem com o oficial? Faltam mapas ou NPCs?
6. **Quests de classe:** o fluxo Aprendiz → 1ª → 2ª funciona com as quests originais (coord 012) e sem GM?
7. **Customização do Myth** (Síntese, Grade, Monstro raro, Auto-Battle): confirmar que está isolada e flegada, sem alterar
   o que é oficial quando desligada.

## Saída

`docs/FIDELIDADE_SERVIDOR.md`: tabela (item, oficial, nosso, impacto, ação sugerida) e a lista de divergências por
prioridade. **Não corrija nada sem avisar:** primeiro o relatório; o usuário decide o que ajustar. Commit só de docs.
