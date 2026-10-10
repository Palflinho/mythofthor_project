status: feito

# 012: receitas dos efeitos feitos em código com `"type": "FUNC"` vazio (aura do nível 99 primeiro)

**De:** Claude · **Data:** 2026-10-10 · **Contexto:** `docs/ROADMAP.md` (F1, itens 1.5 e 1.6) e `docs/FIDELIDADE.md`.

A `Config/skill_effects_full.json` tem várias entradas `"type": "FUNC"` com `"file": ""` e sem `func`. Elas são
efeitos que o executável desenha por código, sem `.str` e sem receita nos specs. O primeiro é o **200**, a aura do
nível 99 (`EF_LEVEL99`, 99_2, 99_3), que o `docs/ref/05` seção 3.6 só descreve em palavras ("anéis azuis girando no chão
e partículas subindo"), sem números.

## Pedido

1. **Aura do nível 99** (`docs/ref/12_efeitos_aura99.md`), com números exatos:
   - quando liga e desliga (o `clevel >= 99` do pacote de unidade? só do jogador ou de todos?), o `/aura` e os três modos;
   - anéis: textura, raio, altura, velocidade de giro, cor, mistura, quantos;
   - partículas: textura, quantidade, taxa, altura final, tempo de vida, velocidade, tamanho;
   - o que muda entre `EF_LEVEL99`, `_2` e `_3`.
2. **Os demais `FUNC` vazios** da tabela (ids 161, 162, 163 e os que você achar na `skill_effects_full.json`): para cada
   um, o que o cliente desenha, ou "nada, é só gatilho" quando for o caso. Liste os ids com a conclusão.
3. Se algum dos 29 `FUNC quake` tiver amplitude ou duração diferente do padrão (50 a 200 ms, ~10 px), anote.

## Regras

As mesmas de `docs/HANDOFF_ANTIGRAVITY_PESQUISA_REFERENCIAS.md`: comportamento e números em português, sem copiar
código GPL, citando a origem (`OM arquivo ~linha`). Commit só de docs.
