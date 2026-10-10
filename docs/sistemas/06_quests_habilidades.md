# Sistema 06: Quests de Habilidades Extras (Platinum Skills)

> **Projeto:** Myth of Thor  
> **Fase do Roadmap:** 7.2 (Conteúdo - Servidor & Quests)  
> **Arquivos do rAthena:**  
> - `server/npc/custom/myth/skill_quests/` (scripts organizados por classe)  
> - `server/npc/custom/myth/skill_quests.conf` (registro no carregamento do map-server)  
> - Pacotes no cliente: `ZC_SKILLINFO_LIST` (`0x010f` / `0x082b`), `ZC_SKILLINFO_UPDATE` (`0x010e`), `ZC_NOTIFY_EFFECT2` (`0x01f3`)

---

## 1. Visão Geral e Filosofia de Design

No Ragnarok Online clássico (Pre-Renewal), cada classe (1ª classe e 2ª classe) possui habilidades especiais que **não consomem pontos da árvore de habilidades** e não podem ser adquiridas pela distribuição comum ao subir de nível. Elas são as chamadas **Habilidades de Quest** (*Platinum Skills*).

Essas habilidades possuem papel crucial na identidade mecânica e no dinamismo das classes:
- **Espadachins** ganham mobilidade e sustentação com *Recuperação de HP em Movimento* e o atordoamento situacional do *Golpe Fatal*.
- **Magos** recebem a camada defensiva vital de sobrevivência *Proteção Energética* (*Energy Coat*).
- **Arqueiros** tornam-se autossuficientes com a habilidade de *Fabricar Flechas* e ganham controle de distância com *Disparo Certeiro* (*Charge Arrow*).
- **Noviços** desbloqueiam sua única habilidade ofensiva pura na 1ª classe: *Luz Sagrada* (*Holy Light*).
- **Ladrões** recebem ferramentas mecânicas icônicas como *Recuar* (*Back Slide*) e *Chutar Areia* (*Sand Attack*).
- **Mercadores** ganham utilidade e dano em área chave com *Cavalo-de-Pau* (*Cart Revolution*), *Personalizar Carrinho* e *Grito de Guerra* (*Crazy Uproar*).
- **2ªs Classes** recebem habilidades de suporte, tática avançada e a lendária *Bioética* (Alquimistas) para criação de Homunculus.

No **Myth of Thor**, as quests são integradas ao sistema nativo de NPCs, respeitando os requisitos pré-renovação de nível de classe e itens clássicos, com diálogos temáticos imersivos executados na nova caixa de diálogo clássica (360×160) do cliente Unreal.

---

## 2. Arquitetura Técnica no Servidor (rAthena)

### 2.1 Concessão Permanente via `SKILL_PERM_GRANT`
No rAthena, habilidades normais possuem `flag = 0` na tabela `skill` do banco MariaDB. Para conceder uma habilidade de quest permanente que:
1. **Não consome Skill Points** do personagem;
2. **Ignora pré-requisitos de árvore** de skills;
3. **Não pode ser esquecida** ao resetar habilidades via resetter/Mesmerist (`@resetskill`);

Utiliza-se a chamada de script com flag de concessão permanente:
```c
skill .@reward_skill_id, .@reward_skill_lv, SKILL_PERM_GRANT; // flag 3
```

### 2.2 Persistência e Variáveis de Quest
Cada quest possui uma variável de escopo de personagem persistente no banco de dados (`char_reg_num_db`):
- Padrão de nomenclatura: `MYTH_QS_<SKILL_ACRONYM>` (exemplo: `MYTH_QS_AUTOBERSERK`, `MYTH_QS_HOLYLIGHT`).
- Valores:
  - `0`: Não iniciada.
  - `1`: Em progresso / coletando materiais.
  - `2`: Concluída com sucesso e habilidade concedida.

### 2.3 Tratamento na Reencarnação (Transclasse - High Novice)
Quando um personagem atinge o nível 99/50 e renasce pelas mãos da Valquíria para se tornar Transclasse:
- Como regra oficial do RO, as habilidades de quest aprendidas na primeira vida são lembradas pelo espírito do guerreiro. Ao mudar para a respectiva classe transclasse (ex: Aprendiz T -> Espadachim T), o NPC mestre da quest (ou a própria transição) reativa a habilidade sem necessidade de gastar os itens novamente se a variável `MYTH_QS_*` for igual a `2`.

---

## 3. Catálogo Completo das Habilidades de Quest

### 3.1 Aprendiz (Novice)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Primeiros Socorros** (`NV_FIRSTAID`) | 142 | 4 | Enfermeira Nami (`prontera 152, 195`) | 3x Trévolo (908), 3x Felpa (914), 1x Erva Vermelha (507) | Recupera 5 de HP consumindo 3 de SP. |
| **Fingir de Morto** (`NV_PLAYDEAD`) | 143 | 7 | Aprendiz Tom (`prt_castle 45, 35`) | Nenhum (conversa e prova de astúcia) | Finge-se de morto; monstros perdem o alvo imediatamente. Exclusivo de Aprendiz. |

### 3.2 Espadachim (Swordsman)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Recuperação de HP em Movimento** (`SM_MOVINGRECOVERY`) | 145 | 35 | Cavaleiro Deon (`izlude_in 74, 172`) | 200x Casca de Besouro (943), 1x Garrafa Vazia (713), 1x Pedaço de Vela (708) | Permite regenerar HP passivamente enquanto se move (25% da taxa parado). |
| **Golpe Fatal** (`SM_FATALBLOW`) | 146 | 30 | Instrutor Leo (`prt_in 75, 107`) | 10x Chifre de Besouro (944), 10x Tira de Espinho (945), 1x Banana (513) | Concede ao Golpe Fulminante nível >= 6 a chance de causar Atordoamento (*Stun*). |
| **Instinto de Sobrevivência** (`SM_AUTOBERSERK`) | 144 | 30 | Mestre Ash (`prt_church 104, 88`) | 35x Laço (722), 10x Pó de Borboleta (923), 1x Suco de Banana (533) | Ativa automaticamente Provocar Nível 10 em si mesmo quando HP cai abaixo de 25%. |

### 3.3 Mago (Mage)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Proteção Energética** (`MG_ENERGYCOAT`) | 157 | 35 | Feiticeiro Raul (`geffen_in 151, 119`) | 1x Conta de Vidro (734), 1x Pena de Pássaro (916), 1x Diamante de 1 Carat (730), 5x Casca de Lagarta (936) | Cria barreira de mana: reduz o dano físico sofrido em até 30% em troca de consumo de SP por golpe. |

### 3.4 Arqueiro (Archer)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Fabricar Flechas** (`AC_MAKINGARROW`) | 147 | 30 | Arqueiro Kyu (`payon_in01 142, 32`) | 20x Tronco (1019), 7x Cristal Azul (704), 1x Pedaço de Vela (708), 1x Casca de Ovo (514) | Transforma itens de inventário em flechas correspondentes (elementais, aço, prata, etc.). |
| **Disparo Certeiro** (`AC_CHARGEARROW`) | 148 | 35 | Caçador Jason (`payon_in02 64, 71`) | 3x Tentáculo (962), 10x Casca de Fruta (949), 2x Pata de Macaco (942) | Dispara flecha potente com dano de 150% que empurra o alvo 6 células para trás. |

### 3.5 Noviço (Acolyte)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Luz Sagrada** (`AL_HOLYLIGHT`) | 156 | 30 | Padre Linus (`prt_church 173, 23`) | 1x Cristal Azul (704), 1x Pérola (726), 1x Rosa Vermelha (748) | Disparo mágico sagrado à distância: 125% de ATQM do elemento Sagrado. Custo 15 SP. |

### 3.6 Ladrão (Thief)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Chutar Areia** (`TF_SANDATTACK`) | 151 | 25 | Ladino Bagu (`moc_prydb1 91, 115`) | 5x Saco de Areia (7035), 1x Pele de Minhoca (928) | Dano físico de 130% do elemento Terra com 20% de chance de causar Cegueira (*Blind*). |
| **Recuar** (`TF_BACKSLIDE`) | 149 | 35 | Ladrão Ágil (`moc_prydb1 105, 120`) | 20x Erva Amarela (508), 10x Espinho de Cacto (953) | Salta instantaneamente 5 células para trás. Sem conjuração, custo 7 SP. |
| **Procurar Pedra** (`TF_FINDSTONE`) | 150 | 20 | Catador (`morocc 182, 60`) | 2x Pedra (7049) | Escava o chão e obtém uma Pedra no inventário (se peso < 50%). |
| **Arremessar Pedra** (`TF_THROWSTONE`) | 152 | 20 | Atirador (`morocc 185, 62`) | Concluir *Procurar Pedra* | Arremessa uma pedra causando 30 de dano fixo (ignora DEF) com 5% de atordoar. |

### 3.7 Mercador (Merchant)

| Habilidade | ID | Job Lv | NPC / Localização | Materiais Exigidos | Efeito Mecânico |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Personalizar Carrinho** (`MC_CHANGECART`) | 154 | 30 | Charron (`alberta_in 114, 219`) | 50x Madeira (1019), 20x Ferro (998), 20x Pedaço de Pele de Animal (919) | Modifica o modelo visual do carrinho entre 5 opções baseadas no nível base do jogador. |
| **Grito de Guerra** (`MC_LOUD`) | 155 | 15 | Mercador Gritador (`alberta 118, 59`) | 7x Pérola (726), 50x Ovas de Sapo (907), 1x Casca de Caranguejo (963) | Bradar de encorajamento: concede +4 de FOR para si por 5 minutos. |
| **Cavalo-de-Pau** (`MC_CARTREVOLUTION`) | 153 | 35 | Mestre do Carrinho (`alberta_in 28, 144`) | 1x Garrafa de Poção (713), 20x Minério de Ferro (1002), 5x Suco de Uva (534) | Dano físico em área ao redor do mercador escalando com o peso do carrinho (até 250% + 150% no peso máximo) empurrando alvos 2 células. |

---

### 3.8 Habilidades de Quest das 2ªs Classes (Seleção Inicial)

| Classe | Habilidade | ID | Job Lv | Localização | Efeito Principal |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Cavaleiro** | Avanço Ofensivo (`KN_CHARGEATK`) | 1001 | 40 | Prontera | Avança contra o alvo à distância; dano aumenta conforme a distância percorrida. |
| **Bruxo** | Visão Mágica (`WZ_SIGHTBLASTER`) | 1006 | 40 | Geffen | Invoca esfera flamejante que repele inimigos atacantes causando dano de fogo. |
| **Caçador** | Flecha Fantasma (`HT_PHANTASMIC`) | 1009 | 40 | Payon | Disparo com arco que não consome flechas, causando 150% de dano neutro e empurrão. |
| **Sacerdote** | Redenção (`PR_REDEMPTION`) | 1014 | 40 | Prontera Santuário | Sacrifica o sacerdote para ressuscitar todos os membros do grupo na tela. |
| **Ferreiro** | Dublê de Corpo (`BS_UNFAIRLYTRICK`) | 1012 | 40 | Geffen | Habilidade passiva que reduz o custo de taxas e serviços de mercadores. |
| **Mercenário** | Aceleração Sônica (`AS_SONICACCEL`) | 1004 | 40 | Morroc Assassin Guild | Aumenta o dano das Lâminas Destruidoras em +10% e sua Precisão em +50. |
| **Mercenário** | Arremessar Adaga (`AS_VENOMKNIFE`) | 1003 | 40 | Morroc Assassin Guild | Permite arremessar adagas venenosas envenenando o alvo à distância. |
| **Cruzado** | Piedade (`CR_SHRINK`) | 1002 | 40 | Prontera Castelo | Quando Auto-Guarda bloqueia um golpe, empurra o atacante 2 células para trás. |
| **Alquimista** | Bioética (`AM_BIOETHICS`) | 238 | 40 | Lighthalzen Rekenber | Habilidade fundamental que desbloqueia a árvore secreta dos Homunculus. |

---

## 4. Integração com a UI do Cliente (Unreal Engine)

1. **Janela de Diálogo Clássica:**
   - As falas utilizam comandos `mes`, `next`, `select` e `close`, integrando-se perfeitamente ao widget de diálogo implementado pelo Claude (`AMythPlayerController` / `UISayDialogWnd`).
2. **Atualização Imediata da Skill Tree:**
   - Ao executar `skill <id>, <lv>, SKILL_PERM_GRANT`, o servidor envia automaticamente o pacote `ZC_SKILLINFO_UPDATE` (`0x010e`):
     - `SkillID`: ID da habilidade concedida.
     - `SkillLevel`: Nível concedido (1).
     - `SpCost`: Custo de SP nativo.
     - `AttackRange`: Alcance.
     - `Upgradable`: `0` (False — a habilidade é permanente e não recebe pontos de evolução).
   - O cliente Unreal insere a habilidade na lista de atalhos e na aba de habilidades sem exibir botões de `+` de evolução.
3. **Efeito Visual e Sonoro:**
   - O comando `specialeffect2 EF_LEVELUP` aciona a coluna clássica de luz dourada com anéis angélicos no pawn do jogador e emite o SFX correspondente.

---

## 5. Implementação no Servidor

### 5.1 Estrutura de Arquivos
```
server/npc/custom/myth/
├── skill_quests.conf
└── skill_quests/
    ├── 00_novice.txt
    ├── 01_swordsman.txt
    ├── 02_mage.txt
    ├── 03_archer.txt
    ├── 04_acolyte.txt
    ├── 05_thief.txt
    ├── 06_merchant.txt
    └── 07_second_jobs.txt
```

### 5.2 Carregamento no rAthena
O arquivo `server/npc/custom/myth/skill_quests.conf` é referenciado no `npc/scripts_custom.conf` do container `mythofthor-map`, recarregável dinamicamente a quente via console GM:
```
@reloadnpc
```
ou pelo envio de pacote de chat GM pelo cliente.
