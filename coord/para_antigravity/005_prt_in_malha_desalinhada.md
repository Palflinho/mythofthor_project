status: feito

# 005: interior `prt_in` com a malha visual desalinhada do grid do `.gat`

**De:** Claude · **Data:** 2026-10-10

## Sintoma (testado ao vivo, prints em `Saved/Screenshots/WindowsEditor/x_03_dentro.png` e `y_01_dentro.png`)

O portal da loja de armas (prontera 133,183) funciona: carrega `Lvl_PrtIn` e o servidor põe o personagem em
`prt_in (50,105)`.

- O cliente posiciona o personagem no chão do `.gat` (colisão `_gat_floor`, canal Visibility) em
  **X=-5425 Y=-525 Z=90**, com `CellOriginUU` = (-7950, 4750) e 50 uu/célula.
- Na tela, o centro (onde está o personagem) fica sobre o **vazio preto**. A sala da loja (balcão, NPC, estantes)
  aparece **deslocada para cima e para a esquerda**.
- Ou seja: a malha visual do `prt_in` não bate com o grid do `.gat` / servidor. Em Prontera bate.

## Tarefa (investigar e propor a correção; não precisa reimportar)

1. Em `tools/ue_import_map.py`:
   - origem: `origin_x, origin_y = -cx * scale, cy * scale`;
   - colisão: `add_gat_collision`.

   Compare como o centro `(cx, cy)` é calculado para a malha visual (vinda do `.gnd`/`.rsw`) e para a colisão do
   `.gat`, em `prontera` e em `prt_in`.
2. Leia os cabeçalhos:
   - do `prt_in.gat` (largura x altura);
   - do `prt_in.gnd` (largura x altura em tiles; 1 tile = 2 células);
   - o mesmo de `prontera`.

   **Hipótese:** em `prt_in`, `gnd*2 != gat`, ou há offset de ladrilhos. Aí o centro usado para uma malha difere do
   da outra.
3. Calcule o deslocamento exato, em células e em uu, para o `prt_in`. Diga se afeta outros interiores importados:
   - `prt_church`;
   - `prt_castle`;
   - qualquer mapa com `gnd*2 != gat`.
4. Proponha a correção no script. O ideal é que a malha visual e a colisão usem a mesma origem. Ela vai ser aplicada
   na próxima reimportação. Diga também como **reimportar** um mapa: o comando do editor/Python que se usa hoje.

**Conta:** se precisar testar no jogo, use a `mythbot`.

Responda em `coord/para_claude/`.
