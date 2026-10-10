# Sistema 02: Grau de Equipamento (Enchant Grade)

> **Projeto:** Myth of Thor  
> **Arquivos do rAthena:**  
> - `db/enchantgrade.yml` (definição de graus, tipos, níveis e custos)  
> - `db/import/enchantgrade.yml` (configuração para Pre-Renewal)  
> - `server/npc/custom/myth/grade_npc.txt` (NPC Mestre de Graus em Prontera)  
> - Pacotes no cliente: `ZC_EQUIPMENT_ITEMLIST` (`0x0b39` - campo `grade`), `ZC_REQ_WEAR_EQUIP_ACK` (`0x0999`)

---

## 1. Comportamento para o Jogador

1. **Conceito de Grau:**
   - Equipamentos de alto nível (Armas Nível 3 e 4, e Armaduras avançadas) que tenham atingido refino mínimo de **+9** podem ser consagrados com um **Grau de Qualidade**:
     - **Grau D (Bronze)**
     - **Grau C (Prata)**
     - **Grau B (Ouro)**
     - **Grau A (Valquíria / Místico)**
2. **Visualização no Cliente:**
   - O grau aparece como um prefixo antes do refino no nome do item:
     - Exemplo: `[Grau D] +9 Espada Larga [2]`
     - Na janela de equipamentos e na descrição (`ItemInfo`), uma insígnia de cor metálica indica o grau atual do item.
3. **Bônus Obtidos por Grau (Balanceados para Pre-Renewal):**
   - **Grau D:** +5 ATQ / +5 ATQM, +3% MaxHP.
   - **Grau C:** +10 ATQ / +10 ATQM, +5% MaxHP, +3 DEF / MDEF.
   - **Grau B:** +18 ATQ / +18 ATQM, +7% MaxHP, +5 DEF / MDEF, +2% Dano Físico/Mágico geral.
   - **Grau A:** +28 ATQ / +28 ATQM, +10% MaxHP, +8 DEF / MDEF, +4% Dano Geral e efeito visual sutil de brilho pulsante no sprite do personagem.
4. **Segurança (Sem Quebra de Item):**
   - O aprimoramento de grau **NUNCA destrói o equipamento**.
   - Em caso de falha: os materiais consumidos são perdidos e o refino pode sofrer uma redução leve de -1 nível se não for usado catalisador protetor.

---

## 2. Requisitos, Chances e Materiais

### 2.1 Requisitos de Entrada
- Equipamento deve ser Arma Nível 3/4 ou Armadura com **Refino $\ge +9$**.

### 2.2 Tabela de Sucesso e Custos

| Transição de Grau | Refino Mínimo | Chance Base | Custo em Zeny | Materiais Necessários | Catalisador de Chance (+10%) |
|---|---|---|---|---|---|
| **Sem Grau $\rightarrow$ Grau D** | +9 | 60% | 250.000z | 5x Éter Menor, 2x Oridecon Puro | 1x Cristal Concentrado |
| **Grau D $\rightarrow$ Grau C** | +10 | 45% | 500.000z | 10x Éter Menor, 5x Oridecon Puro | 2x Cristal Concentrado |
| **Grau C $\rightarrow$ Grau B** | +11 | 30% | 1.000.000z | 5x Éter Maior, 1x Oridecon Enriquecido | 3x Cristal Místico |
| **Grau B $\rightarrow$ Grau A** | +12 | 15% | 2.500.000z | 10x Éter Maior, 2x Oridecon Enriquecido | 5x Cristal Místico |

---

## 3. Compatibilidade de Protocolo e PACKETVER

- O suporte ao campo `grade` no struct `EQUIPITEM_INFO` exige obrigatoriamente:
  `PACKETVER_MAIN_NUM >= 20200916`
- Como atualizamos nosso servidor para **`PACKETVER 20250402`** na Tarefa 001, o protocolo já suporta o campo `uint8 grade;` nativamente logo após `uint8 RefiningLevel;` no pacote `0x0b39` (`ZC_EQUIPMENT_ITEMLIST`), sem necessidade de gambiarras em strings ou scripts!

---

## 4. Comunicação com o Cliente

1. **Campo `grade` no Pacote:**
   - Byte com valor de `0` (nenhum), `1` (Grau D), `2` (Grau C), `3` (Grau B), `4` (Grau A).
2. **Exibição na HUD:**
   - `ItemInfo` desenha a tag de cor correspondente:
     - `Grau D`: Bronze `#cd7f32`
     - `Grau C`: Prata `#c0c0c0`
     - `Grau B`: Ouro `#ffd700`
     - `Grau A`: Platina/Místico `#e5e4e2` com aura brilhante.
