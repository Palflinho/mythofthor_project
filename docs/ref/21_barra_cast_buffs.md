# Referência 21: Barra de Conjuração (Cast Bar), Interrupção e Ícones de Buffs/Debuffs

> **Fonte:** Open-Midgard (`src/world/GameActor.cpp`, `src/ui/UIRechargeGage.cpp`), roBrowserLegacy (`src/Renderer/Entity/EntityLife.js`, `src/UI/Components/StatusIcons/StatusIcons.js`, `src/Engine/MapEngine/Skill.js`), rAthena (`src/map/skill.cpp`, `src/map/status.cpp`).  
> **Escopo:** Fase 2 (F2) — Indicadores temporais de feitiçaria, regras estritas de interrupção e gerenciamento de ícones de status da HUD.  
> **Prioridade:** P1 (Fidelidade do ciclo de conjuração e visibilidade de buffs).

---

## 1. Resumo

A barra de conjuração (*Cast Bar* / `UIRechargeGage`) informa a duração do tempo de preparo de feitiços e habilidades do personagem. Ela flutua sobre a cabeça do conjurador enquanto anéis rúnicos mágicos giram aos seus pés. No cliente original, a conjuração é vulnerável: qualquer dano físico ou mágico sofrido pelo personagem interrompe imediatamente o feitiço, cancelando a barra e colocando a habilidade em falha. No topo direito da tela, a HUD organiza os ícones de bônus (*buffs*) e penalidades (*debuffs*) ativos, com contadores regressivos e alertas visuais de expiração.

---

## 2. Dados e Constantes

### 2.1. Barra de Conjuração Flutuante (`UIRechargeGage`)
- **Dimensões:** **$60 \times 6$ px**.
- **Posição Relativa:** Centralizada horizontalmente sobre o topo do sprite do personagem, $+10$ px acima da linha do nome/clã.
- **Cor do Preenchimento:** Verde esmeralda brilhante (`RGB(0, 220, 80)` ou `#00DC50`) com borda preta sólida de 1 px.
- **Efeito Visual no Solo:** Círculo concêntrico de runas procedurais (`EF_CASTING` ou anéis elementais de fogo, gelo, vento ou terra) sob os pés do conjurador.

### 2.2. Fórmula Oficial de Redução do Tempo de Conjuração (Pre-Renewal 1:1)
O tempo de preparo da habilidade é reduzido puramente pelo atributo de Destreza (DEX) do personagem:
$$\text{TempoFinal} = \text{TempoBase} \times \max\left(0.0, \; 1.0 - \frac{\text{DEX}}{150}\right)$$
- Com **150 de DEX**, o tempo de conjuração é reduzido a **0 ms** (a lendária *"Conjuração Instantânea"* / *Instacast* do pre-renewal).
- A barra de conjuração nunca surge na tela se $\text{TempoFinal} < 200$ ms.

### 2.3. Painel de Ícones de Status na HUD (`StatusIcons`)
- **Localização:** Canto superior direito da tela, logo abaixo da moldura do minimapa.
- **Dimensões do Ícone:** **$24 \times 24$ px** (ícones de texturas `efst_*.bmp`).
- **Espaçamento:** Grade de 2 a 3 colunas, alinhadas da direita para a esquerda com espaçamento de 4 px entre ícones.
- **Formatação do Tooltip:** Ao passar o mouse sobre o ícone, surge o balão descritivo contendo:
  - Nome do Buff ou Debuff (ex: *Aumentar Agilidade Nv. 10*).
  - Descrição resumida dos bônus concedidos.
  - Tempo restante formatado como `Minutos : Segundos` (ex: `04:32`).
- **Alerta de Expiração:** Nos últimos **5 segundos** de duração, o ícone pisca com frequência de 2 Hz (duas vezes por segundo com transparência oscilando entre 30% e 100%).

---

## 3. Regras de Comportamento

### 3.1. Ciclo de Vida da Conjuração
1. O jogador seleciona uma habilidade com tempo de preparo e clica no alvo ou no solo.
2. O cliente envia a requisição de habilidade para o servidor (`CZ_USE_SKILL` ou `CZ_USE_SKILL2`).
3. O servidor valida SP, alcance e catalisadores, respondendo com `ZC_USESKILL_ACK` contendo o ID da habilidade e o `castTime` em milissegundos.
4. **Reação do Cliente:**
   - O ator entra no estado de conjuração (`kGameActorCastingStateId`).
   - A barra de $60 \times 6$ px é criada sobre o personagem com valor inicial 0%.
   - Um preenchimento linear avança de 0% a 100% de acordo com a interpolação temporal de milissegundos:
     $$\text{Progresso}(t) = \text{clamp}\left(\frac{\text{TickAtual} - \text{TickInicio}}{\text{castTime}}, \; 0.0, \; 1.0\right)$$
   - Enquanto a barra preenche, comandos de movimento no chão ou WASD são bloqueados.

### 3.2. Regras Estritas de Interrupção de Conjuração
No Ragnarok Online clássico:
- Se o personagem sofrer **qualquer ataque físico bem-sucedido ou mágico que cause dano $\ge 1$** durante o período em que a barra estiver na tela:
  - O servidor cancela o feitiço e transmite a notificação de interrupção.
  - A barra de conjuração é destruída imediatamente.
  - O anel mágico no solo se dissipa com centelhas quebradas.
  - O personagem entra na animação de dor/reação a impacto (*Hit Reaction*).
  - O SP gasto na tentativa **não é devolvido**.
- **Exceções Legítimas à Interrupção:**
  1. **Habilidade Vigor (*Endure* — `SM_ENDURE`):** Permite resistir a até 7 impactos consecutivos sem sofrer interrupção na conjuração.
  2. **Carta Fen (ID 4066):** Torna as conjurações do portador 100% ininterruptíveis por dano, penalizando o tempo de preparo em **+25%**.
  3. **Carta Borboleta Sanguinária (ID 4141):** Conjurações ininterruptíveis com penalidade de **+30%** no tempo de preparo.

---

## 4. Principais Ícones de Status e Identificadores

| ID Oficial rAthena | Nome do Status | Origem | Duração Típica | Efeito Mecânico |
|---|---|---|:---:|---|
| `SI_BLESSING` (10) | Bênção (*Blessing*) | Habilidade Noviço | 240 s (Nv 10) | +10 STR, +10 INT, +10 DEX |
| `SI_INC_AGI` (12) | Aumentar Agilidade | Habilidade Noviço | 240 s (Nv 10) | +12 AGI, +25% velocidade de movimento |
| `SI_ANGELUS` (9) | Angelus | Habilidade Noviço | 300 s (Nv 10) | +250% Soft DEF de Vitalidade |
| `SI_MAGNIFICAT` (14) | Magnificat | Habilidade Sacerdote | 90 s (Nv 5) | Dobra velocidade de recuperação de HP/SP |
| `SI_GLORIA` (21) | Glória | Habilidade Sacerdote | 30 s (Nv 5) | +30 LUK |
| `SI_KYRIE` (19) | Kyrie Eleison | Habilidade Sacerdote | 120 s (Nv 10) | Barreira que bloqueia até 10 golpes ou 46% do HP Máx |
| `SI_TWOHANDQUICKEN` (1) | Rapidez com Duas Mãos | Habilidade Cavaleiro | 300 s (Nv 10) | +30% de ASPD com espadas de 2 mãos |
| `SI_ADRENALINE` (23) | Adrenalina Pura | Habilidade Ferreiro | 150 s (Nv 5) | +30% de ASPD com maças e machados |
| `SI_OVERTHRUST` (25) | Força Excessiva | Habilidade Ferreiro | 100 s (Nv 5) | +25% de dano na arma (0,1% chance de quebrar) |
| `SI_POISON` (883) | Envenenamento (*Debuff*) | Dano venenoso | 30 s | Reduz DEF em 25%, drena 3% de HP a cada 3s |
| `SI_STUN` | Atordoamento (*Debuff*) | Impacto físico | 2 a 5 s | Personagem imóvel, incapaz de esquivar e agir |

---

## 5. Entradas e Saídas (Protocolo de Pacotes)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_USESKILL_ACK` (`0x013e`):** Notifica o início de conjuração com `{ skillId, castTime, targetGID }`.
- **`ZC_NOTIFY_SKILL_CANCEL`:** Notifica que a conjuração foi cancelada por dano recebido ou perda de alvo.
- **`ZC_SET_UNIT_STATUS` / `ZC_STATUS_CHANGE` (`0x0119` / `0x043f`):** Concede ou remove um buff/debuff na HUD `{ statusId, durationMs, flag }`.

---

## 6. Como Testar

1. **Teste de Barra de Cast:** Com um Mago, conjurar *Lanças de Fogo Nv 10*. A barra verde de $60 \times 6$ px deve surgir sobre o personagem e preencher suavemente durante os segundos de conjuração.
2. **Teste de Interrupção:** Durante a conjuração de *Lanças de Fogo*, receber um ataque de monstro. A barra deve sumir imediatamente, a magia deve falhar e o personagem deve reagir ao golpe.
3. **Teste de Ininterruptibilidade com Carta Fen ou Vigor:** Ativar a habilidade *Vigor* e conjurar a mesma magia. Sofrer até 7 ataques sem que a barra seja cancelada.
4. **Ícone de Buff na HUD:** Receber o buff *Aumentar Agilidade*. O ícone de sapato alado deve surgir abaixo do minimapa. Passar o mouse sobre ele: o tooltip deve indicar o nome e contagem regressiva de tempo. Nos últimos 5 segundos, o ícone deve piscar.
