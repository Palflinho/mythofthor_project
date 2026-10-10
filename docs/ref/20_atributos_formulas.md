# Referência 20: Atributos Primários, Secundários e Fórmulas da Janela de Status

> **Fonte:** roBrowserLegacy (`src/UI/Components/WinStats/WinStatsCommon.js`), Open-Midgard (`src/ui/UIStatusWnd.cpp`), rAthena (`src/map/pc.cpp`, `src/map/status.cpp`, `conf/battle/status.conf`).  
> **Escopo:** Fase 2 (F2) — Janela de atributos (`Alt+A`), progressão por pontos, fórmulas de combate e formatação visual "Base + Bônus".  
> **Prioridade:** P1 (Progressão de personagem e fidelidade matemática 1:1).

---

## 1. Resumo

A Janela de Status (`UIStatusWnd` / `Alt+A`) exibe a ficha completa do personagem, dividida entre os 6 atributos primários (FOR, AGI, VIT, INT, DES, SOR) e os atributos secundários derivados de combate (ATK, MATK, DEF, MDEF, HIT, FLEE, CRITICAL e ASPD). O jogador investe Pontos de Atributo (*Status Points*) acumulados ao subir de nível de base. A escala de custo é não-linear progressiva. Todos os atributos primários são formatados como $\text{Base} + \text{Bônus}$, onde a Base reflete os pontos investidos e o Bônus reúne equipamentos, cartas, bônus de classe de nível de classe e magias ativas.

---

## 2. Dados e Constantes

### 2.1. Dimensões da Janela de Status
- **Atalho de Teclado:** `Alt+A` (alterna visibilidade).
- **Dimensões Clássicas:** $280 \times 320$ px.
- **Posição Inicial Padrão:** $(0, 233)$ ou centralizado à esquerda.

### 2.2. Tabela de Custo de Pontos por Atributo (Pre-Renewal Oficial)

O custo em Pontos de Atributo para aumentar um atributo de $N$ para $N+1$ segue a fórmula:
$$\text{Custo}(N) = \left\lfloor \frac{N - 1}{10} \right\rfloor + 2$$

| Valor Atual do Atributo Base ($N$) | Custo para o Próximo Ponto |
|:---:|:---:|
| **1 a 11** | **2 pontos** |
| **12 a 21** | **3 pontos** |
| **22 a 31** | **4 pontos** |
| **32 a 41** | **5 pontos** |
| **42 a 51** | **6 pontos** |
| **52 a 61** | **7 pontos** |
| **62 a 71** | **8 pontos** |
| **72 a 81** | **9 pontos** |
| **82 a 91** | **10 pontos** |
| **92 a 99** | **11 pontos** |

> **Limite Máximo Clássico:** O valor base de qualquer atributo primário não pode ultrapassar **99** (ou 120 no Renewal).

---

## 3. Fórmulas Oficiais dos Atributos Secundários (Pre-Renewal 1:1)

### 3.1. Poder de Ataque Físico (`ATK` — Exibido como: $\text{Base} + \text{Arma}$)
Na janela de status do cliente clássico, o ATK é representado em dois números: $\text{ATK}_{\text{base}} + \text{ATK}_{\text{equip}}$.
- **Ataque Base (Corpo-a-corpo):**
  $$\text{ATK}_{\text{base}} = \text{STR} + \left\lfloor \frac{\text{STR}}{10} \right\rfloor^2 + \left\lfloor \frac{\text{DEX}}{5} \right\rfloor + \left\lfloor \frac{\text{LUK}}{5} \right\rfloor$$
  *(Note o bônus quadrático a cada 10 pontos fechados de STR).*
- **Ataque Base (Armas de Longo Alcance — Arcos, Armas de Fogo e Instrumentos):**
  $$\text{ATK}_{\text{base}} = \text{DEX} + \left\lfloor \frac{\text{DEX}}{10} \right\rfloor^2 + \left\lfloor \frac{\text{STR}}{5} \right\rfloor + \left\lfloor \frac{\text{LUK}}{5} \right\rfloor$$
- **Ataque de Equipamento:** Soma do poder de ataque da arma equipada, refinamento e bônus de cartas.

---

### 3.2. Poder de Ataque Mágico (`MATK` — Exibido como: $\text{Mínimo} \sim \text{Máximo}$)
No pre-renewal, o dano mágico possui uma faixa estocástica entre valor mínimo e máximo:
$$\text{MATK}_{\min} = \text{INT} + \left\lfloor \frac{\text{INT}}{7} \right\rfloor^2$$
$$\text{MATK}_{\max} = \text{INT} + \left\lfloor \frac{\text{INT}}{5} \right\rfloor^2$$
- A janela de status exibe explicitamente: `140 ~ 196`.

---

### 3.3. Defesa Física (`DEF` — Exibida como: $\text{Equip} + \text{VIT}$)
- **DEF de Equipamento (Hard DEF — Redução Percentual):**
  - Soma da defesa básica das armaduras e escudos equipados + bônus de refino.
  - Cada 1 ponto de Hard DEF reduz o dano físico sofrido em **1%**.
- **DEF de Vitalidade (Soft DEF — Redução Subtrativa Direta):**
  $$\text{SoftDEF} = \text{VIT} \times 0.8 + \text{rnd}\left(0, \; \max\left(0, \; \left\lfloor \frac{\text{VIT}}{20} \right\rfloor^2 - 1\right)\right)$$
- Exibição na janela: `45 + 52` (45% de redução percentual e -52 de dano fixo absorvido).

---

### 3.4. Defesa Mágica (`MDEF` — Exibida como: $\text{Equip} + \text{INT}$)
- **MDEF de Equipamento (Hard MDEF):** Reduz o dano mágico sofrido em 1% por ponto.
- **MDEF de Inteligência (Soft MDEF):**
  $$\text{SoftMDEF} = \text{INT} + \left\lfloor \frac{\text{VIT}}{2} \right\rfloor + \left\lfloor \frac{\text{DEX}}{5} \right\rfloor$$
- Exibição na janela: `12 + 68`.

---

### 3.5. Precisão (`HIT`)
Chance de acertar o golpe físico contra a esquiva do monstro:
$$\text{HIT} = \text{NívelDeBase} + \text{DEX}$$

---

### 3.6. Esquiva (`FLEE` — Exibida como: $\text{EsquivaNormal} + \text{EsquivaPerfeita}$)
- **Esquiva Normal:**
  $$\text{FLEE}_{\text{base}} = \text{NívelDeBase} + \text{AGI}$$
- **Esquiva Perfeita (Lucky Dodge — representada como `+ N`):**
  - Chance percentual pura e direta de ignorar completamente qualquer golpe físico, independente do HIT do atacante:
  $$\text{EsquivaPerfeita} = 1 + \left\lfloor \frac{\text{LUK}}{10} \right\rfloor$$
- Exibição na janela: `210 + 5` (210 de FLEE normal e 5% de Esquiva Perfeita).

---

### 3.7. Taxa de Ataque Crítico (`CRITICAL`)
Chance percentual de desferir golpe crítico (que ignora a FLEE e a DEF do alvo):
$$\text{CRITICAL} = 1 + \left( \text{LUK} \times 0.3 \right) + \text{BônusCartas}$$

---

### 3.8. Velocidade de Ataque (`ASPD`)
Determina a cadência de golpes por segundo.
- No pacote do rAthena, a velocidade vem em centésimos de delay de ataque (`amotion` em ms).
- **Conversão para a Tela da Janela de Status:**
  $$\text{ASPD}_{\text{display}} = \left\lfloor 200 - \frac{\text{amotion}}{10} \right\rfloor$$
- Escala: varia de **130** (lento, maças pesadas sem agilidade) até o teto clássico de **190** (ou 193 no Renewal), correspondendo a até 5 golpes por segundo.

---

## 4. Regras de Comportamento da Interface

1. **Botões de Incremento (`+`):**
   - Ao lado de cada atributo primário, existe um botão azul com sinal de `+`.
   - O botão é desenhado em estado ativo apenas se o jogador possuir Pontos de Atributo suficientes para pagar o custo daquele atributo (`statuspoint >= Custo(N)`).
   - Ao clicar no botão, o cliente envia `CZ_STATUS_CHANGE` (`0x00bc`) contendo o ID do atributo (13 = STR, 14 = AGI, 15 = VIT, 16 = INT, 17 = DEX, 18 = LUK) e a quantidade incrementada (+1).
2. **Atualização Dinâmica dos Bônus:**
   - Quando o jogador sobe o Nível de Classe (*Job Level*), a tabela de bônus de classe (`job_bonus_db`) concede bônus fixos que são somados na parcela direita (`+ Bônus`) sem gastar pontos de atributo.
   - Buffs como Bênção (`AL_BLESSING`: +10 STR, +10 INT, +10 DEX) ou Aumentar Agilidade (`AL_INCAGI`: +12 AGI) somam imediatamente ao valor de bônus, recalculando instantaneamente o ATK, HIT, FLEE e ASPD na interface.

---

## 5. Como Testar

1. **Abertura da Janela:** Pressionar `Alt+A`. Verificar se a janela abre com os 6 atributos alinhados à esquerda e atributos secundários à direita.
2. **Custo de Pontos:** Criar um personagem nível 1 com 48 pontos de atributos disponíveis. Elevar a Força de 1 a 11 (custo 2 cada) e verificar se no nível 12 o custo exigido sobe para 3 pontos.
3. **Cálculo de ATK:** Subir a FOR de 9 para 10. O ATK base deve receber um salto quadrático perceptível (+1 pelo ponto e bônus da dezena fechada).
4. **ASPD:** Equipar poção da concentração (+10% ASPD) ou buff de AGI: conferir se o número de ASPD sobe proporcionalmente (ex: de 156 para 162).
