status: pendente

# Resposta 011: Efeitos das Skills — Bloco 3: Transclasses (Conclusão da Tarefa 008)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 008 (Bloco 3: Transclasses)

---

## 1. Conclusão da Tarefa 008

Com esta entrega, a **Tarefa 008 está 100% concluída** em todos os três blocos (1ª Classe, 2ªs Classes 2-1 e 2-2, e Transclasses)!

1. **`Config/skill_effects_full.json` Completo:**
   - **651 habilidades mapeadas**, cobrindo 100% das classes oficiais clássicas até transclasse.
   - **1.356 efeitos e projéteis indexados**, garantindo que nenhum efeito fique mudo ou sem textura.
   - Mapeamento rigoroso de estágios (`cast`, `effect`, `hit`, `ground`), elemento (`Neutral`, `Water`, `Earth`, `Fire`, `Wind`, `Poison`, `Holy`, `Ghost`, `Weapon`), e sons associados.
2. **`docs/ref/11_efeitos_procedurais.md` Integralmente Atualizado:**
   - Seção 2: 1ª Classe (Bash, Magnum Break, Heal, Bolts, Inc AGI, Blessing, Endure, etc.).
   - Seção 3: 2ªs Classes 2-1 e 2-2 (Grand Cross, Asura Strike, Auto Guard, Sanctuary, Magnus Exorcismus, Fire Pillar, Jupitel, Waterball, Sonic Blow, etc.).
   - Seção 4: Transclasses (Glória Domini / Pressure, Berserk, Spiral Pierce, Gospel, Basilica, Assumptio, EDP, Acid Demonstration, Cart Termination, Falcon Assault, etc.).
   - Seção 5: Guia de arquitetura e shaders GPU na Unreal Engine.

---

## 2. Destaques Procedurais das Transclasses

- **Glória Domini (Pressure — ID 367 / Efeito 365):**
  - Cilindro monumental de luz sagrada descendo a prumo do céu (`poszStart: 35.0` até o chão, `height: 30.0` células, `duration: 1200 ms`) com cruz no solo e tremor de terra (`Camera.setQuake: 200 ms`).
- **Frenesi (Berserk / Fury — IDs 359/360 / Efeito 368):**
  - Cilindro aditivo em chamas de sangue carmesim (`ring_red.tga`, `height: 8.0`, `duration: 2000 ms`, `animation: 2`) + rugido de batalha.
- **Perfurar em Espiral (Spiral Pierce — ID 397 / Efeito 339):**
  - Vórtice cônico espiral em alta rotação disparado no alvo com 5 impactos sonoros sucessivos.
- **Basílica (ID 362 / Solo 374) & Canto de Batalha (Gospel — ID 369 / Solo 370):**
  - Cúpulas e anéis de amplo raio (9x9 células) com auras protetoras e colunas de bênção em loop.
- **Assumptio (ID 361 / Efeito 440):**
  - Escudo sagrado alado flutuante com centelhas e halo de partículas.
- **Encantar com Veneno Mortal (EDP — ID 378 / Efeito 493):**
  - Vórtice púrpura tóxico em torno das lâminas do algoz (`ring_black` matizado em violeta ácido).
- **Bomba Ácida (Acid Demonstration — ID 490 / Efeito 537):**
  - Arremesso balístico duplo cruzado de Frasco de Ácido + Frasco de Fogo Grego com detonação mista de estilhaços e névoa corrosiva.
- **Choque de Carrinho (Cart Termination — ID 485 / Efeito 518):**
  - Impacto devastador do carrinho com chuva quádrupla de faíscas metálicas `lens1.tga` e repulsão física.

---

## 3. Próximo Foco

Marcamos a **Tarefa 008 como `status: feito`**.  
Estamos partindo imediatamente para a **Tarefa 009: Fase 3 — "Jogável Mínimo"**:
1. **Item 3.6:** Mapeamento de todas as Quests e NPCs de Troca de Classe (1ª e 2ª classe e Valquíria de Rebirth no rAthena pre-re, com mapa, coordenadas e requisitos de script do cliente como `input`, `cutin`, `viewpoint`).
2. **Item 3.8:** Importação dos mapas das cidades (`izlude`, `geffen`, `payon`, `morocc`, `alberta`), interiores de guilda e primeiros campos e calabouços com centro corrigido pelo `.gat` e dilatação de magenta.
