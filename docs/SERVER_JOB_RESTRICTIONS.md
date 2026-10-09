# Restrições de Classes no Servidor (Myth of Thor)

> **Decisão do Projeto:** O jogo Myth of Thor é estritamente limitado até as **Classes Transcendentais** (2-1a, 2-2a) e classes expandidas clássicas. 3ª e 4ª classes **não** fazem parte do jogo.

Este documento formaliza as garantias e configurações ativas no servidor rAthena (WSL/Docker) que asseguram esta diretriz.

---

## 1. Configuração do rAthena (Pre-Renewal)

1. **Modo de Compilação do Servidor:**
   - O servidor rAthena foi configurado com flags `--enable-prere=yes --enable-packetver=20211103`.
   - No modo Pre-Renewal, o arquivo primário carregado pelo `map-server` é `npc/pre-re/scripts_main.conf`.

2. **Isolamento de Scripts de Mudança de Classe:**
   - O arquivo `npc/re/scripts_jobs.conf` (que contém os NPCs de mudança para 3ª classe: Cavaleiro Rúnico, Arcebispo, Sicário, etc.) **não** é importado no Pre-Renewal.
   - O arquivo `npc/scripts_jobs.conf` contém exclusivamente:
     - **1-1 e Expandidas:** Espadachim, Mago, Arqueiro, Noviço, Mercador, Gatuno, Justiceiro, Ninja, Taekwon.
     - **2-1:** Cavaleiro, Bruxo, Caçador, Sacerdote, Ferreiro, Mercenário.
     - **2-1 Transcendentais:** Lorde, Sumo Sacerdote, Arquimago, Algoz, Atirador de Elite, Mestre-Ferreiro.
     - **2-1 Expandida:** Mestre Taekwon.
     - **2-2:** Cavaleiro, Bruxo, Templário, Bardo, Odalisca, Monge, Arruaceiro, Sábio.
     - **2-2 Transcendentais:** Paladino, Menestrel, Cigana, Mestre, Desordeiro, Professor, Criador.
     - **2-2 Expandida:** Espiritualista.
     - **Aprendiz / Superaprendiz:** Aprendiz clássico e Superaprendiz.
     - **Transcendência:** Valquíria (`npc/jobs/valkyrie.txt`), permitindo renascer do nível 99 para Aprendiz T.

3. **Scripts Customizados:**
   - O script `jobmaster.txt` (mestre de classes customizado de teste) está explicitamente **desativado** (`//npc: npc/custom/jobmaster.txt`) em `npc/scripts_custom.conf`.
   - Não há NPCs de auto-job que ofereçam opções de 3ª ou 4ª classe.

4. **Itens e Mecânicas de Mudança de Classe:**
   - O banco de dados de itens (`db/pre-re/item_db.yml`) não contém itens consumíveis que alterem a classe do jogador para 3ª classe.
   - As árvores de evolução no banco de dados Pre-Renewal encerram-se nas classes transcendentais.

5. **Comandos GM:**
   - O comando administrativo `@job` permanece acessível exclusivamente para contas com permissão de GM (nível 99) para propósitos de teste e depuração.
