status: feito

# 008: tabela completa skill → efeito + receita dos efeitos desenhados pelo executável (prioridade alta)

**De:** Claude · **Data:** 2026-10-10 · **Pedido do usuário** ("usei o Impacto Explosivo e não saiu a animação")

## Situação

`Config/skill_effects.json` mapeia efeito para só **61 de 431 skills**. O Impacto Explosivo (skill 7) tem só
`element: Fire`; o efeito 17 tem só o som `ef_magnumbreak`.

No cliente, muitos efeitos são **procedurais** (desenhados pelo executável, sem .str). Exemplos:

- Magnum Break;
- Bash;
- Heal;
- Bolts;
- Fire Ball;
- Napalm Beat;
- Soul Strike;
- Increase AGI / Blessing;
- Teleport;
- Endure;
- Provoke...

O portal e os números de dano já fiz pela docs/ref/05.

## Tarefa (pesquisa nos clientes de referência; não copie código GPL, descreva dados e comportamento)

1. **Tabela skill → efeitos** (RB `src/DB/Skills/SkillEffect.js` e similares; OM equivalente) para **todas as skills
   das classes até transclasse** (ids 1..~400 + 1001..1020 + as de trans). Para cada skill:
   - efeito no conjurador ao começar o cast;
   - efeito no início da skill (`effectId`);
   - efeito no alvo / acerto (`hitEffectId`);
   - efeito no chão (`groundEffectId`);
   - som.

   Entregue como **JSON** em `Config/skill_effects_full.json`, no mesmo formato do `skill_effects.json` atual. Mantenha
   as chaves que eu já leio: `skills[id]` com `effect` / `hit` / `ground` / `element`, e `effects[id]` com uma
   lista de partes `{type, file, wav, attached}`.
2. **Receita de cada efeito procedural** usado por essas skills. No RB, `EffectTable.js`, os tipos que não são STR:
   - `3D`;
   - `2D`;
   - `CYLINDER`;
   - `SPR`;
   - `FUNC`.

   Escreva `docs/ref/11_efeitos_procedurais.md` com, por efeito:
   - textura;
   - blend;
   - tamanho inicial/final;
   - altura;
   - duração (ms);
   - rotação/velocidade;
   - cor/alpha ao longo do tempo;
   - quantas cópias;
   - se segue o alvo;
   - atraso.

   Use os campos que o RB define (`duration`, `alphaMax`, `fadeIn/Out`, `posz`, `size`, `sizeStart/End`, `rotate`,
   `toSrc`, `repeat`, `animation`...).

   **Prioridade:** primeiro as skills de Aprendiz, Espadachim, Mago, Arqueiro, Noviço, Mercador e Gatuno; depois a 2ª
   classe e a trans.
3. **Entregue em partes,** uma resposta por bloco em `coord/para_claude/`:
   - 1ª classe;
   - 2ª classe;
   - trans.

   Eu começo a implementar assim que chegar a primeira.
