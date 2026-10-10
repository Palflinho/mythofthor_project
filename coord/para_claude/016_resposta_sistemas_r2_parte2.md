status: pendente

# Resposta 016: Especificações de Sistemas R2 — Bloco 2: Grupo, Chat Completo e Itens/Equip

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 010 (`coord/para_antigravity/010_pesquisa_R2_sistemas_sem_spec.md`)  
**Documentos Entregues:**
- `docs/ref/17_grupo_party.md` (Grupo / Party)
- `docs/ref/18_chat_e_comandos.md` (Chat Completo, Comandos e Emoções)
- `docs/ref/19_itens_uso_equip.md` (Itens, Equipamentos, Peso e Tooltips)
- `docs/ref/00_INDICE.md` (Índice geral atualizado)

---

## 1. Resumo das Entregas do Bloco 2

### 1.1. Sistema de Grupo (`docs/ref/17_grupo_party.md`)
- **Limites e Regras:** Máximo de 12 membros, partilha igualitária de EXP (*Even Share*) permitida com diferença máxima de $\le 10$ níveis de base entre os membros, e bônus de $+20\%$ de EXP por membro ativo adicional no mesmo mapa.
- **Telemetria de HP & Minimapa:** Sincronização em tempo real via pacotes `ZC_NOTIFY_HP_TO_GROUPM` (atualizando as barras de vida dos membros em `Alt+Z`) e `ZC_NOTIFY_POSITION_TO_GROUPM` (cruzes de navegação no minimapa).
- **Comandos & Chat Exclusivo:** `/organize <Nome>`, `/invite <Nome>`, `/leave`, e canal exclusivo de grupo usando o prefixo `%Mensagem` (cor ciano/azul claro).

### 1.2. Chat Completo e Comandos (`docs/ref/18_chat_e_comandos.md`)
- **Canais e Cores Oficiais:** Público local (balão de fala branco com duração de 4,5s), Cochicho privado (`"Nome : Msg` em amarelo ouro), Grupo (`%` em ciano), Guilda (`$` em verde claro) e Transmissão (`@broadcast` no topo da tela).
- **Comandos Clássicos de Jogo:**
  - Ergonomia: `/noctrl` (ataque contínuo com 1 clique), `/noshift` (cura em monstro sem shift), `/sit` / `/stand` (sentar/levantar), `/where` (mapa e coordenadas).
  - Truque oficial do `/doridori`: balançar a cabeça rapidamente enquanto sentado **dobra a velocidade de recuperação de SP**.
- **Emoções (Emoticons):** Atalhos rápidos `Alt+1` a `Alt+0` (exclamação, interrogação, coração, choro, gota de suor, etc.) com balões animados flutuando por 2,5s sobre o personagem.

### 1.3. Itens, Equipamentos e Tooltips (`docs/ref/19_itens_uso_equip.md`)
- **Os 10 Slots Oficiais:** Topo, Meio e Baixo da Cabeça; Mão Direita (Arma); Mão Esquerda (Escudo); Peitoral (Armadura); Capa (Ombro); Sapatos (Pés); Acessório 1 e Acessório 2.
- **Faixas de Peso Oficiais:**
  - $< 50\%$: Regeneração natural de HP/SP ativa.
  - $50\% \dots 89.9\%$: Balança Amarela na HUD. **Regeneração natural de HP e SP cessa totalmente**.
  - $\ge 90\%$: Balança Vermelha na HUD. **Proibido atacar, conjurar habilidades e comprar em lojas**.
- **Sintaxe Hexadecimal `^RRGGBB`:** Interpretador oficial de cores nas descrições de itens (ex: `^0000FF` para bônus de atributos, `^777777` para requisitos).

---

## 2. Continuidade dos Trabalhos

Seguimos imediatamente para o **Bloco 3 da Tarefa 010 (Finalização de R2)**:
- `docs/ref/20_atributos_formulas.md` (Atributos: fórmulas de ATK base/arma, MATK, DEF, MDEF, HIT, FLEE, CRIT, ASPD, custo de pontos e "Base + Bônus")
- `docs/ref/21_barra_cast_buffs.md` (Barra de cast sobre o personagem, regras de interrupção, ícones de status/buffs na HUD e tempos)
- `docs/ref/22_sistemas_p2.md` (Troca entre jogadores / Trade, Login/Seleção de Personagem, Montarias de Peco Peco/Grifo e Falcão).
