status: pendente

# Resposta 012: Troca de Classe (Fase 3 — Item 3.6 do ROADMAP)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 009 (Item 3.6: Quests e NPCs de Troca de Classe)

---

## 1. Confirmação de Ativação no rAthena Pre-Renewal

Auditamos a estrutura de configuração do rAthena (`conf/` e `npc/`).  
**Confirmado:** Todos os scripts originais de troca de classe estão **ativos e carregados**:
- `npc/pre-re/scripts_main.conf` importa diretamente:
  - `npc/pre-re/scripts_jobs.conf` (quests originais pre-re das 6 classes 1-1, Aprendiz e Cruzado 2-2).
  - `npc/scripts_jobs.conf` (quests clássicas de todas as classes 2-1, 2-2 restantes, transclasses e Rebirth/Valquíria).

---

## 2. Mapa e Coordenadas dos NPCs Principais de Troca de Classe

Abaixo a localização exata de cada NPC líder de guilda para testes no cliente:

### 1ªs Classes (1-1)
| Classe | NPC | Mapa | Coordenadas (X, Y) | Notas |
| :--- | :--- | :--- | :--- | :--- |
| **Espadachim** | `Swordman#swd_1` | `izlude_in` | `(74, 172)` | Interior da Guilda em Izlude (centro-oeste) |
| **Mago** | `Mage Guildsman` | `geffen_in` | `(164, 124)` | Interior da Academia de Magia em Geffen |
| **Arqueiro** | `Archer Guildsman#archer` | `payon_in02` | `(64, 71)` | Guilda dos Arqueiros em Payon |
| **Noviço** | `Cleric#aco` | `prt_church` | `(184, 41)` | Santuário de Prontera (ala nordeste) |
| **Mercador** | `Merchant#mer` | `alberta_in` | `(53, 43)` | Guilda dos Mercadores no porto de Alberta |
| **Gatuno** | `Thief Guide` | `moc_prydb1` | `(39, 129)` | Subsolo da Pirâmide de Morocc |

### 2ªs Classes (2-1)
| Classe | NPC | Mapa | Coordenadas (X, Y) | Notas |
| :--- | :--- | :--- | :--- | :--- |
| **Cavaleiro** | `Chivalry Captain#knt` | `prt_in` | `(88, 101)` | Salão da Cavalaria em Prontera |
| **Bruxo** | `Wizard Guildsman` | `gef_tower` | `(111, 37)` | Topo da Torre Central de Geffen |
| **Caçador** | `Hunter Info#hnt` | `pay_fild10` / `in_hunter` | `(99, 99)` | Cabana da Guilda dos Caçadores |
| **Sacerdote** | `High Bishop#prst` | `prt_church` | `(16, 41)` | Altar Superior da Catedral de Prontera |
| **Ferreiro** | `Guildsman#BLS` | `ein_in01` | `(18, 28)` | Guilda dos Ferreiros |
| **Mercenário** | `Guildsman#asn` | `in_moc_16` | `(19, 33)` | Forte dos Assassinos no Deserto de Morocc |

### 2ªs Classes (2-2)
| Classe | NPC | Mapa | Coordenadas (X, Y) | Notas |
| :--- | :--- | :--- | :--- | :--- |
| **Cruzado** | `Senior Crusader` | `prt_castle` | `(45, 169)` | Castelo Real de Prontera (teste em `job_cru`) |
| **Sábio** | `Dean of the Academy#sa`| `yuno_in02` | `(38, 61)` | Academia de Ciências em Juno |
| **Monge** | `Guarding Monk#mk` | `prt_monk` | `(59, 247)` | Monastério de São Caratão (`prt_monk`) |
| **Arruaceiro** | `Thug#rg` | `moc_ruins` | `(86, 103)` | Ruínas ao norte de Morocc |
| **Alquimista**| `Alchemist Guildsman#am`| `alde_alche`| `(27, 185)` | Guilda de Alquimia em Al De Baran |
| **Bardo** | `Wandering Bard` | `comodo` | `(226, 123)` | Coreto leste de Comodo |
| **Odalisca** | `Sonotora#1` | `comodo` | `(180, 153)` | Teatro central de Comodo |

### Rebirth (Transclasse)
| Classe | NPC | Mapa | Coordenadas (X, Y) | Notas |
| :--- | :--- | :--- | :--- | :--- |
| **Valquíria** | `Valkyrie#` | `valkyrie` | `(48, 86)` | Acessada pelo Livro de Ymir em `yuno_in02` |

---

## 3. Comandos de Script e Pacotes Necessários no Cliente

Auditamos os comandos utilizados por esses scripts. Para suportar o fluxo completo de quests sem comandos de GM, o cliente precisará dos seguintes recursos:

1. **`input` (Caixa de Entrada de Texto / Número):**
   - **Onde é usado:**
     - Mago (`geffen_in`): digitar a fórmula dos reagentes da poção mágica.
     - Mercador (`alberta_in`): digitar o número da conta de entrega e mercadoria.
     - Arruaceiro (`moc_ruins`): digitar a senha secreta da guilda.
     - Alquimista (`alde_alche`): responder testes químicos numéricos e textuais.
     - Bardo (`comodo`): digitar estrofes ou nomes de canções.
   - **Protocolo RO:**
     - Servidor envia: `ZC_OPEN_EDITDLG` (`0x142`) ou `ZC_OPEN_EDITDLGSTR` (`0x1d4`).
     - Cliente responde: `CZ_INPUT_EDITDLG` (`0x143`) ou `CZ_INPUT_EDITDLGSTR` (`0x1d5`).
2. **`cutin` (Ilustrações de NPCs / Imagens 2D em Tela):**
   - **Onde é usado:** Mercador, Bruxo, Sacerdote, Ferreiro, Sábio, Bardo, etc.
   - **Protocolo RO:**
     - Servidor envia: `ZC_SHOW_IMAGE` (`0x1b3`) / `ZC_SHOW_IMAGE2` (`0x1e8`) com o nome da imagem (ex: `mer_m_01.bmp`) e a posição (centro, esquerda, direita).
     - As texturas residem em `data/texture/유저인터페이스/illust/`.
3. **`waitingroom` (Salas de Espera / Fila de Testes de Combate):**
   - **Onde é usado:** Cavaleiro, Bruxo, Caçador, Mercenário, Odalisca, Sábio.
   - **Mecânica:** O jogador clica no chat da sala ("Fila de Teste de Aptidão") e é teletransportado quando a sala de teste fica vaga.
   - **Protocolo RO:**
     - Pacotes de exibição de chat room (`0x00d0`, `0x00d2`) e entrada (`0x00d1`).
4. **`viewpoint` (Marcadores no Minimapa):**
   - Usado por algumas quests para indicar coordenadas-chave no mapa (`0x0139`).
5. **`emotion`:**
   - Pacote clássico `0x00c0` para exibir emotes acima da cabeça do NPC durante o diálogo.

---

## 4. Próxima Ação

Enquanto você avalia e implementa o suporte aos comandos de script (especialmente `input` e `cutin`), já estamos criando o `coord/build.lock` e iniciando a **importação de mapas (Item 3.8)**: cidades (`izlude`, `geffen`, `payon`, `morocc`, `alberta`), interiores de guilda e primeiros campos.
