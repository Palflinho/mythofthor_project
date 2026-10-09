# Arquitetura de Quests de Habilidade e Levantamento de Habilidades 3rd (Myth of Thor)

Este documento descreve a infraestrutura criada para as **quests de habilidades customizadas por classe** e apresenta um levantamento técnico de habilidades de 3ª classe que podem ser adaptadas para classes pré-renovação / transcendentais sem alterações no código C++ do rAthena.

---

## 1. Estrutura de Arquivos das Quests de Habilidade

Todas as quests customizadas residem na pasta `server/npc/custom/myth/skill_quests/` (espelhada em `~/mythofthor-server/rathena/npc/custom/myth/skill_quests/`):

| Arquivo | Finalidade |
|---|---|
| `skill_quest_example.txt` | Exemplo funcional completo em Prontera com validação de classe, nível de classe, itens e concessão permanente |
| `TEMPLATE_skill_quest.txt` | Modelo parametrizado pronto para ser copiado e preenchido para novas quests de habilidade |
| `skill_quests.conf` | Arquivo de inclusão que carrega todas as quests da pasta no `scripts_custom.conf` |

---

## 2. Mecânica de Concessão de Habilidade (`SKILL_PERM_GRANT`)

No rAthena, o comando de script `skill` aceita flags que determinam como a habilidade é adicionada à ficha do jogador (`src/map/pc.hpp` e `src/map/script_constants.hpp`):

```c
skill <skill_id>, <skill_level>, SKILL_PERM_GRANT;
```

- **`SKILL_PERM` (0 / `ADDSKILL_PERMANENT`):** Concede a habilidade permanentemente, porém valida os pré-requisitos da árvore de habilidades da classe.
- **`SKILL_PERM_GRANT` (3 / `ADDSKILL_PERMANENT_GRANTED`):** **Recomendado para quests extras e habilidades adaptadas.** Concede a habilidade de forma permanente ignorando a árvore de habilidades e pré-requisitos, permitindo que a habilidade seja aprendida mesmo que não conste originalmente na árvore clássica daquela classe.

---

## 3. Controle e Progresso de Quests

Para rastrear o progresso e impedir repetição indesejada:

1. **Variável de Personagem (Simples e Direto):**
   ```c
   // Ao entregar os itens:
   setd "MYTH_SKILLQUEST_IGNITION", 1;
   
   // Ao conversar com o NPC:
   if (getd("MYTH_SKILLQUEST_IGNITION") == 1 || getskilllv(2006) > 0) {
       mes "Você já aprendeu esta habilidade.";
       close;
   }
   ```

2. **Sistema de Quests do Cliente Clássico (Alt+U):**
   - Utilizando `setquest <quest_id>;` no início e `completequest <quest_id>;` no fim, registrando a entrada no `db/quest_db.yml` para aparecer no livro de missões do jogador.

---

## 4. Levantamento de Habilidades de 3ª Classe Adaptáveis (Pre-Renewal)

Todas as habilidades abaixo já constam no motor de combate do rAthena (`battle.cpp`, `skill.cpp`) e estão configuradas com fórmulas e custos em `db/pre-re/skill_db.yml`. Elas funcionam **imediatamente sem qualquer modificação em C++**, bastando concedê-las via quest (`skill <ID>, <Nível>, SKILL_PERM_GRANT`).

> [!NOTE] Habilidades que exigem montarias exclusivas (Dragão do RK, Lobo/Warg do Ranger, Grifo do RG, Robô Madogear) ou itens especiais de Renewal (Tintas do Renegado, Runas rúnicas) foram excluídas ou marcadas, pois demandariam suporte a mecânicas adicionais. As habilidades listadas abaixo dependem exclusivamente de armas convencionais, SP ou itens clássicos.

### ⚔️ Cavaleiro / Lorde (Rune Knight)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2006` | `RK_IGNITIONBREAK` | Impacto Explosivo | Explosão de dano físico em área (5x5) ao redor do conjurador. Dano aumentado com Espada de Duas Mãos e elemento da arma. |
| `2004` | `RK_WINDCUTTER` | Lâmina de Vento | Golpe de vento em área causando empurrão e chance de infligir Medo. Requer espada de duas mãos ou lança. |
| `2007` | `RK_SONICWAVE` | Onda de Choque | Disparo cortante à distância em alvo único com alto dano de impacto. |
| `2005` | `RK_DEATHBOUND` | Vínculo da Morte | Contra-ataque concentrado que absorve e devolve o dano do adversário multiplicado. |

### 🛡️ Templário / Paladino (Royal Guard)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2307` | `LG_SHIELDPRESS` | Compressão de Escudo | Golpe esmagador com o escudo com dano escalando pelo peso e refino do escudo. Chance de atordoamento. |
| `2309` | `LG_PINPOINTATTACK` | Ataque Preciso | Investida rápida de lança à distância que acerta ponto vital, podendo quebrar equipamento e causar Sangramento. |
| `2310` | `LG_RAGEBLASTER` | Disparo de Fúria | Descarga em área liberando a fúria acumulada em combate. |

### 📖 Sacerdote / Sumo Sacerdote (Arch Bishop)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2041` | `AB_HIGHNESSHEAL` | Curatio (Cura Alta) | Versão superpoderosa da Cura com multiplicador substancial. |
| `2040` | `AB_CHEAL` | Coluceo Heal | Cura coletiva em área curando todos os membros do grupo visíveis simultaneamente. |
| `2045` | `AB_PRAEFATIO` | Praefatio | Aplica o efeito de Kyrie Eleison instantaneamente a todos os membros do grupo na tela. |
| `2044` | `AB_RENOVATIO` | Renovatio | Buff de regeneração contínua que restaura 3% do HP máximo a cada 5 segundos (duração 90s). Em mortos-vivos, causa dano gradual. |
| `2042` | `AB_JUDEX` | Judex | Feitiço sagrado ofensivo de alvo único com respingo mágico (splash) ao redor do alvo. |
| `2043` | `AB_ADORAMUS` | Adoramus | Nuke sagrado devastador que cega e reduz drasticamente a AGI do inimigo (consome Gema Azul). |
| `2046` | `AB_EPICLESIS` | Epiclesis | Invoca a Árvore da Vida, ressuscitando aliados mortos na área e concedendo aura de regeneração de HP/SP e bônus de MaxHP. |

### 🔮 Bruxo / Arquimago (Warlock)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2213` | `WL_CHAINLIGHTNING` | Corrente Elétrica | Relâmpago em cadeia que salta repetidamente entre alvos próximos causando dano de Vento cumulativo. |
| `2211` | `WL_CRIMSONROCK` | Cometa Escarlate | Meteoro gigantesco caindo na área selecionada causando forte impacto de Fogo e Atordoamento. |
| `2216` | `WL_FROSTMISTY` | Névoa Congelante | Névoa gélida que atinge todos os inimigos na tela com chance de Congelamento / Resfriamento. |
| `2217` | `WL_JACKFROST` | Geada Instantânea | Detona todos os inimigos afetados por frio/gelo na tela com dano de Água maciço. |
| `2212` | `WL_EARTHSTRAIN` | Fenda Terrestre | Onda de colunas de terra que viaja em linha reta à frente causando dano de Terra e chance de remoção de elmo/escudo. |
| `2202` | `WL_RADIUS` | Expansão Arcana | Passiva que amplia o alcance de todas as magias e reduz o tempo de conjuração variável. |

### 🗡️ Mercenário / Algoz (Guillotine Cross)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2022` | `GC_CROSSIMPACT` | Impacto Cruzado | Investida de 7 golpes em velocidade extrema em alvo único. |
| `2025` | `GC_ROLLINGCUTTER` | Lâminas de Vento | Giro contínuo com katar que acumula contadores de rotação (até 10). |
| `2026` | `GC_CROSSRIPPERSLASHER` | Retaliação Dilacerante | Dispara lâminas cortantes à distância liberando os contadores de rotação acumulados. |
| `2020` | `GC_DARKILLUSION` | Passo Sombrio | Teleporte instantâneo para trás do alvo inimigo com chance de ativar combo automático. |
| `2024` | `GC_PHANTOMMENACE` | Ameaça Fantasma | Golpe circular de katar que revela todos os inimigos ocultos/escondidos ao redor. |

### 🏹 Caçador / Atirador de Elite (Ranger)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2234` | `RA_ARROWSTORM` | Tempestade de Flechas | Disparo para o alto que faz chover centenas de flechas sobre uma ampla área (11x11). |
| `2233` | `RA_AIMEDBOLT` | Disparo Certeiro | Tiro de precisão que desfere múltiplos impactos extras caso o alvo esteja imobilizado (Presilha ou Armadilha). |
| `2238` | `RA_CLUSTERBOMB` | Armadilha de Fragmentação | Armadilha de alto rendimento explosivo (consome 2x Armadilha de Caça). |
| `2237` | `RA_CAMOUFLAGE` | Camuflagem | Oculta o atirador no ambiente enquanto mantém a capacidade de locomoção, elevando ATQ e Crítico. |

### 🥊 Monge / Mestre (Sura)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2326` | `SR_DRAGONCOMBO` | Combo do Dragão | Golpe inicial de 2 socos que inicia a sequência de combos avançados. |
| `2327` | `SR_FALLENEMPIRE` | Ruína Ancestral | Continuação do combo que esmaga o alvo contra o chão causando paralisia temporária. |
| `2328` | `SR_TIGERKNUCKLEFIST` | Punho do Tigre | Finalizador veloz que projeta o monge contra o alvo. |
| `2331` | `SR_RAMPAGEBLASTER` | Explosão Espiritual | Consome todas as esferas espirituais para desencadear onda de choque explosiva em área. |
| `2336` | `SR_WINDMILL` | Rasteira | Golpe giratório rasteiro que força todos os inimigos ao redor a se sentarem / caírem no chão. |

### 🔨 Ferreiro / Mestre-Ferreiro (Mechanic)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2253` | `NC_AXEBOOMERANG` | Bumerangue de Machado | Arremessa o machado à distância com dano escalando pelo peso do machado e empurrão. |
| `2254` | `NC_AXETORNADO` | Tornado de Machado | Giro violento de machado desferindo 2 golpes em área com dano ampliado pelo atributo Vento/Fogo. |
| `2255` | `NC_POWERSWING` | Golpe Poderoso | Golpe descendente brutal com machado que tem chance de acionar Bumerangue de Machado automaticamente. |

### 🌀 Sábio / Professor (Sorcerer)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2445` | `SO_PSYCHIC_WAVE` | Onda Psíquica | Ondas telecinéticas que pulsam no chão causando dano mágico neutro em múltiplos hits. |
| `2447` | `SO_DIAMONDDUST` | Pó de Diamante | Tempestade de gelo em área com chance de infligir Cristalização. |
| `2448` | `SO_WARMER` | Aquecimento | Santuário térmico no chão que remove congelamento/cristalização e regenera HP continuamente. |
| `2449` | `SO_VARETYR_SPEAR` | Lança de Varetyr | Lança elétrica caindo dos céus causando dano físico e mágico de Vento simultaneamente. |
| `2443` | `SO_STRIKING` | Encanto de Ataque | Buff que energiza as armas do grupo conferindo ATQ plano e taxa de Crítico elevada. |

### 🎭 Arruaceiro / Desordeiro (Shadow Chaser)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2467` | `SC_FATALMENACE` | Ameaça Fatal | Teleporte agressivo até o alvo desferindo golpe em área com adagas. |
| `2469` | `SC_FEINTBOMB` | Clone Explosivo | Deixa uma cópia de palha no local que explode após breve atraso enquanto o personagem recua oculto. |
| `2474` | `SC_TRIANGLESHOT` | Disparo Triplo | Disparo de arco disparando 3 flechas simultâneas com empurrão. |

### 🎵 Bardo & Odalisca / Menestrel & Cigana (Minstrel / Wanderer)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2414` | `WM_METALLICSOUND` | Ruído Metálico | Onda sonora de alta frequência que causa dano acústico mágico e drena SP do alvo. |
| `2412` | `WM_REVERBERATION` | Ressonância | Esfera sonora estacionária no chão que explode por proximidade causando dano físico e mágico híbrido. |
| `2417` | `WM_GREAT_ECHO` | Grande Coral | Coro ensurdecedor em área de grande raio atingindo múltiplos alvos. |

### 🧪 Alquimista / Criador (Genetic)
| Skill ID | Nome Técnico | Nome em Jogo | Descrição / Funcionamento |
|---|---|---|---|
| `2480` | `GN_CARTCANNON` | Canhão de Carrinho | Dispara projéteis do carrinho causando explosão em área (requer bola de canhão clássica ou pedra). |
| `2477` | `GN_THORNS_TRAP` | Armadilha de Espinhos | Trepadeiras com espinhos que brotam do solo prendendo o alvo e causando dano periódico. |
| `2478` | `GN_BLOOD_SUCKER` | Sanguessuga | Planta parasita anexada ao alvo que suga HP gradualmente transferindo para o criador. |
| `2476` | `GN_CRAZYWEED` | Erva Daninha | Ervas gigantes que destroem armadilhas e habilidades de chão inimigas na área de impacto. |
