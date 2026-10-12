# Checkup dos efeitos de skill (classes até as trans)

Gerado por `tools/audit_skill_effects.py` (WSL: `python3 -I tools/audit_skill_effects.py`), cruzando `Config/skill_tree.json`
com `Config/skill_effects_full.json` (extraído da EffectTable/SkillEffect do roBrowser, que é o que o RO original usa).

Resultado (452 skills do escopo): 187 com efeito visual, 113 só com som, 152 sem nada (a maioria passivas).

- **Sem nada e ativas (~65)**: quase todas são utilitárias sem visual no RO (Teleporte, Portal, Identificar Item, Fabricar Flechas,
  Espíritos de classe, Posturas do Taekwon, Criar Monstro Planta...). Se o RO não tem tabela de efeito para elas, o cliente também não.
- **Só som (~110)**: buffs e canções (o visual vem do status/aura, não da tabela de skill), e skills que o RO desenha só pela
  animação do personagem (Assalto do Falcão 381, Perfurar em Espiral 397, Chute do Tornado 413...).
  - Assalto do Falcão e Ataque Aéreo: o cliente faz o falcão voar até o alvo (AMythFalcon::AttackTo), igual ao RO.
- Itens a conferir em jogo (ainda não verificados visualmente): skills de lança (Pierce 56, Brandish 57, Spearstab 58,
  Spear Boomerang 59, Bowling Bash 62), anel de trava (lock-on) sobre monstro real, pontas de gelo da Rajada Congelante (pequenas).
- Passivas (Perícia com..., Aumentar Recuperação...) não têm visual por definição.
