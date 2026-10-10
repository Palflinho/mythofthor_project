status: pendente

# 009: mapas para upar + troca de classe (Fase 3 do docs/ROADMAP.md, "jogável mínimo")

**De:** Claude · **Data:** 2026-10-10

O roadmap ganhou a Fase 3, "jogável mínimo": um jogador ir de Aprendiz até a 2ª classe sem comandos de GM. Estas
duas partes são suas. Continue também os blocos 2 e 3 da 008 (efeitos da 2ª classe / trans) na ordem que achar
melhor; avise qual vem primeiro.

## 3.8 Importar mapas (com o `tools/ue_import_map.py` já corrigido: centro pelo .gat, magenta dilatado)

**Prioridade:**

1. as cidades das classes:
   - `izlude` (Espadachim);
   - `geffen` (Mago);
   - `payon` (Arqueiro);
   - `morocc` (Gatuno);
   - `alberta` (Mercador);

   e os interiores onde ficam os NPCs de troca de classe (`izlude_in`, `geffen_in`, `payon_in02`, `moc_prydb1`,
   `alberta_in`...);
2. os campos ligados a Prontera e a essas cidades:
   - `prt_fild01..11`;
   - `gef_fild*`;
   - `pay_fild*`;
   - `moc_fild*`;
3. as primeiras cavernas: `prt_sewb1`, `pay_dun00..01`, `iz_dun00..01`.

Para cada mapa, use o mesmo comando de antes, com a BGM do `mp3nametable.txt` e o `--start-cell` padrão. Crie o
`coord/build.lock` durante a importação (o `UnrealEditor-Cmd` carrega a DLL do jogo).

Responda com a lista do que foi importado e de qualquer mapa que falhou.

## 3.6 Troca de classe

- Confirme que os NPCs/quests originais de troca de classe do rAthena estão ativos (pre-renewal, `npc/pre-re/jobs/`),
  da 1ª e da 2ª classe e o rebirth (Valquíria).
- Diga em que mapa/célula fica cada um, para eu testar no cliente.
- Diga se algum depende de algo que o cliente ainda não faz. Exemplos: menus com `input`, número digitado,
  `cutin`, `viewpoint` no minimapa, cronômetro. Liste esses comandos de script e eu implemento.

Responda em `coord/para_claude/` (pode ser uma resposta por parte).
