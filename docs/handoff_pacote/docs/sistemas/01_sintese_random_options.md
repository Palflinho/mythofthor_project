# Sistema 01: Síntese de Equipamentos (Atributos Aleatórios - Random Options)

> **Projeto:** Myth of Thor  
> **Arquivos do rAthena:**  
> - `db/item_randomopt_db.yml` (definição de IDs e scripts de bônus)  
> - `db/import/item_randomopt_db.yml` (importação pré-renewal ativa)  
> - `db/item_randomopt_group.yml` (tabelas de grupos, faixas e chances)  
> - `db/import/item_randomopt_group.yml` (grupos customizados de síntese)  
> - `server/npc/synthesis.txt` (NPC de Síntese e Reroll em Prontera)  
> - Pacotes no cliente: `ZC_EQUIPMENT_ITEMLIST` (`0x0b39`), `ZC_ITEM_PICKUP_ACK` (`0x0b41`), `ZC_REQ_WEAR_EQUIP_ACK` (`0x0999`)

---

## 1. Comportamento para o Jogador

1. **Atributos Aleatórios em Drops e Forja:**
   - Equipamentos obtidos no mundo (drops de monstros comuns, elites e chefes) ou sintetizados via NPC podem possuir de **1 a 3 atributos aleatórios** (*Random Options*).
   - Ao inspecionar o item (`ItemInfo` / botão direito), os bônus são exibidos em linhas destacadas em azul/ciano logo abaixo da descrição padrão do item, informando com precisão o atributo e o valor obtido.
2. **Filosofia de Balanceamento Pre-Renewal:**
   - As opções não quebram o balanceamento clássico de atributos (sem multiplicadores absurdos que descaracterizem o combate pré-renovação de 99/50).
   - Não há opções de redução de 100% de pós-conjuração ou dano imune. Valores são incrementais e complementares aos atributos base e cartas.
3. **O NPC de Síntese e Reforja (Prontera):**
   - O ferreiro místico de síntese oferece duas operações:
     - **Imbuir Síntese:** Adiciona 1 a 3 atributos aleatórios a um equipamento limpo consumindo materiais (Cristal de Síntese, Oridecon/Elunium e Zeny).
     - **Reforja (Reroll):** Permite rolar novamente os atributos aleatórios de um item já imbuído, preservando o nível de refino, as cartas equipadas e o encantamento prévio.

---

## 2. Algoritmo, Fórmulas e Números Exatos

### 2.1 Distribuição de Quantidade de Linhas

Ao gerar um item com grupo de síntese:

| Quantidade de Opções | Chance no Drop Comum | Chance no Monstro Elite | Chance no NPC de Síntese |
|---|---|---|---|
| **1 Opção** | 70% | 40% | 55% |
| **2 Opções** | 25% | 45% | 35% |
| **3 Opções** | 5% | 15% | 10% |

### 2.2 Grupos de Síntese e Faixas de Valor

#### Grupo A: Armas Físicas (`ROG_MYTH_WEAPON_PHYS`)
- Aceito em: Espadas, Adagas, Machados, Lanças, Maças, Arcos, Katars, Punhos.

| Opção (`Option`) | ID | Faixa de Valor | Peso / Chance Relativa |
|---|---|---|---|
| `VAR_STRAMOUNT` | 3 | +1 a +5 FOR | 15% |
| `VAR_AGIAMOUNT` | 4 | +1 a +5 AGI | 15% |
| `VAR_DEXAMOUNT` | 7 | +1 a +5 DES | 15% |
| `VAR_ATTACKPERCENT` | 17 | +1% a +7% ATQ Físico | 15% |
| `VAR_CRITICALSUCCESSVALUE` | 19 | +2 a +8 Crítico | 15% |
| `VAR_ATTACKRANGE` | 21 | +5 a +20 Precisão (HIT) | 15% |
| `VAR_ASPDPERCENT` | 23 | +1% a +5% ASPD | 10% |

#### Grupo B: Armas Mágicas (`ROG_MYTH_WEAPON_MAGIC`)
- Aceito em: Cajados de 1M e 2M, Varinhas, Livros.

| Opção (`Option`) | ID | Faixa de Valor | Peso / Chance Relativa |
|---|---|---|---|
| `VAR_INTAMOUNT` | 6 | +1 a +6 INT | 25% |
| `VAR_DEXAMOUNT` | 7 | +1 a +4 DES | 20% |
| `VAR_MAGICATTACKPERCENT` | 18 | +1% a +8% ATQM | 25% |
| `VAR_MAXSPAMOUNT` | 2 | +30 a +150 SP Máximo | 15% |
| `VAR_SPREGENRATE` | 12 | +5% a +15% Regen de SP | 15% |

#### Grupo C: Armaduras e Escudos (`ROG_MYTH_ARMOR_DEF`)
- Aceito em: Armaduras, Vestimentas, Mantos e Escudos.

| Opção (`Option`) | ID | Faixa de Valor | Peso / Chance Relativa |
|---|---|---|---|
| `VAR_MAXHPAMOUNT` | 1 | +100 a +600 HP Máximo | 25% |
| `VAR_VITAMOUNT` | 5 | +1 a +4 VIT | 20% |
| `VAR_DEFAMOUNT` | 25 | +2 a +8 Defesa Física (DEF) | 20% |
| `VAR_MDEFAMOUNT` | 26 | +1 a +5 Defesa Mágica (MDEF) | 15% |
| `VAR_HPREGENRATE` | 11 | +5% a +20% Regen de HP | 20% |

#### Grupo D: Capas e Calçados (`ROG_MYTH_GARMENT_SHOES`)
- Aceito em: Capas, Sapatos, Botas.

| Opção (`Option`) | ID | Faixa de Valor | Peso / Chance Relativa |
|---|---|---|---|
| `VAR_AVOIDSUCCESSVALUE` | 20 | +3 a +12 Esquiva (FLEE) | 25% |
| `VAR_AGIAMOUNT` | 4 | +1 a +4 AGI | 20% |
| `VAR_MAXHPAMOUNT` | 1 | +50 a +350 HP Máximo | 20% |
| `VAR_MAXSPAMOUNT` | 2 | +20 a +100 SP Máximo | 20% |
| `VAR_PERFECTAVOIDSUCVAL` | 22 | +1 a +4 Esquiva Perfeita | 15% |

#### Grupo E: Acessórios (`ROG_MYTH_ACCESSORY`)
- Aceito em: Anéis, Broches, Brincos, Luvas, Colares, Rosários.

| Opção (`Option`) | ID | Faixa de Valor | Peso / Chance Relativa |
|---|---|---|---|
| Atributo Único (+FOR, +AGI, +VIT, +INT, +DES, +SOR) | 3–8 | +1 a +3 | 60% (10% cada) |
| `VAR_MAXHPAMOUNT` | 1 | +50 a +250 HP Máximo | 20% |
| `VAR_MAXSPAMOUNT` | 2 | +20 a +80 SP Máximo | 20% |

---

## 3. Custos do NPC de Síntese (Prontera)

- **Materiais Necessários:**
  - Armas Nv 1 e 2: 1x Oridecon, 2x Fragmento de Síntese, 25.000 Zeny.
  - Armas Nv 3 e 4: 2x Oridecon, 5x Fragmento de Síntese, 100.000 Zeny.
  - Armaduras / Equipamentos de Defesa: 2x Elunium, 4x Fragmento de Síntese, 75.000 Zeny.
- **Reroll de Atributos:**
  - Mesmos materiais + taxa adicional de 50.000 Zeny por tentativa.
  - Taxa de sucesso da operação: 100% (o item **nunca quebra** na síntese).

---

## 4. Comunicação com o Cliente (Unreal Engine 5)

1. **Pacote de Lista de Equipamentos (`ZC_EQUIPMENT_ITEMLIST` - opcode `0x0b39`):**
   - O struct `EQUIPITEM_INFO` contém:
     - `uint8 option_count;` (0 a 5)
     - `struct ItemOptions option_data[MAX_ITEM_OPTIONS];`
   - Onde cada entrada de opção possui:
     - `int16 index;` (ID da opção no enum, ex: 1 = MaxHP, 3 = STR)
     - `int16 value;` (Valor numérico, ex: 500 para +500 HP)
     - `uint8 param;` (Parâmetro secundário, padrão 0)
2. **Exibição na Interface:**
   - No `MythRoItemInfo.cpp`, se `option_count > 0`:
     - O cliente itera sobre as opções e adiciona linhas formatadas em cor ciano (`#5cd6ff`):
       - `[Síntese] FOR +3`
       - `[Síntese] ATQ Físico +5%`
       - `[Síntese] Esquiva +8`
