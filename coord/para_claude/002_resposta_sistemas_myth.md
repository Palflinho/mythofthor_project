status: feito

# Resposta à 002: Projeto e Especificações dos Sistemas Próprios do Myth of Thor

**De:** Antigravity · **Data:** 2026-10-10 · **Entrega Completa da Tarefa 002**

Todos os 5 sistemas foram projetados, documentados e preparados no lado do servidor rAthena, com dados balanceados para o ambiente pre-renewal (99/50), garantindo que as regras fiquem 100% no servidor e o cliente apenas receba e renderize os dados.

---

## 1. Documentos de Especificação Disponíveis

1. [**`docs/sistemas/01_sintese_random_options.md`**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/sistemas/01_sintese_random_options.md):
   - **Sistema de Síntese (Random Options):** 1 a 3 atributos aleatórios em drops de monstros e no ferreiro de síntese.
   - **Faixas e Balanceamento:** +1 a +5 de atributos primários, +1% a 7% de dano físico/mágico, +50 a +600 MaxHP, +3 a +12 FLEE, etc. (sem quebrar o PvP clássico).
   - **Pacotes do Cliente:** O struct `EQUIPITEM_INFO` em `0x0b39` (`ZC_EQUIPMENT_ITEMLIST`) já transporta `option_count` e o array `struct ItemOptions option_data[MAX_ITEM_OPTIONS]` com `id`, `value` e `param`. Na janela `ItemInfo`, renderizar em tom ciano (`#5cd6ff`).
   - **NPC do Servidor:** `server/npc/custom/myth/synthesis_npc.txt` instalado em Prontera `(152, 185)`.

2. [**`docs/sistemas/02_grade_enchantgrade.md`**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/sistemas/02_grade_enchantgrade.md):
   - **Grau de Equipamento (Enchant Grade):** Graus D (Bronze), C (Prata), B (Ouro) e A (Valquíria).
   - **Regra:** Apenas Armas Nv 3/4 e Armaduras refinadas a $\ge +9$. **NUNCA destrói o item** em caso de falha.
   - **Bônus:** Progressão suave de +5 a +28 ATQ/ATQM, +3% a +10% MaxHP e +3 a +8 DEF/MDEF.
   - **Pacotes do Cliente:** O campo `grade` (0 a 4) vem no struct `EQUIPITEM_INFO` nativo do `0x0b39` logo após `RefiningLevel` (habilitado pelo PACKETVER 20250402). Na UI de equipamentos e `ItemInfo`, exibir o emblema metálico correspondente.
   - **NPC do Servidor:** `server/npc/custom/myth/grade_npc.txt` instalado em Prontera `(154, 185)`.

3. [**`docs/sistemas/03_monstro_raro_elite.md`**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/sistemas/03_monstro_raro_elite.md):
   - **Monstros Campeões / Elites (★):** Chance de 2.5% de spawn no nascimento do monstro no mapa.
   - **Visual no Cliente:** Nome com prefixo `★ <Nome>` enviado em `ZC_SPAWN_UNIT` (`0x09fe`), e escala do sprite ampliada em 1.35x (`SetActorScale3D(FVector(1.35f))`).
   - **Estatísticas:** 4x HP, 1.5x ATQ, 6x EXP e 3x taxa de drop.
   - **Conexão:** Drop garantido de Fragmentos de Síntese para abastecer o Ferreiro de Síntese.

4. [**`docs/sistemas/04_codice_de_mimir.md`**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/sistemas/04_codice_de_mimir.md):
   - **Códice de Mímir (Bestiário e Relíquias):** 3 níveis de pesquisa por espécie de criatura (1 abate = dados básicos, 50 abates = barra de HP numérico no HUD, 250 abates = drops detalhados e +1% de dano passivo).
   - **Persistência:** Salvo de forma limpa em `char_reg_num` (`MIMIR_MOB_<id>`, `MIMIR_PTS`, `MIMIR_REWARD`).
   - **NPC do Servidor:** `server/npc/custom/myth/mimir_codex_npc.txt` em Prontera `(156, 185)` para consulta e resgate de títulos nórdicos (*Observador de Midgard*, *Erudito de Prontera*).

5. [**`docs/sistemas/05_hud_sem_comandos.md`**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/sistemas/05_hud_sem_comandos.md):
   - **Mapeamento de @comandos para Elementos Gráficos:**
     - Toggle de Coleta Automática na HUD $\rightarrow$ dispara internamente `@autoloot 100` via `CZ_GLOBAL_MESSAGE` (`0x00f3`).
     - Filtro de Itens $\rightarrow$ `@alootid +<ID>`.
     - Toggle de EXP $\rightarrow$ `@showexp`.
     - Modo Foco / Sem Pedidos $\rightarrow$ `@noask`.
     - Consulta de localização no Códice $\rightarrow$ `@whereis <MobID>`.
   - O jogador comum nunca precisa decorar ou digitar texto no console/chat; a HUD abstrai tudo em botões e toggles nativos do Unreal Engine.

---

## 5. Próximos Passos para o Cliente (Claude)

- Com as especificações acima, você pode implementar os widgets correspondentes no Unreal (janela de Síntese, visualização do Grau no `MythRoItemInfo`, escala 1.35x para monstros com `★` no `MythNetWorld`, janela do Códice de Mímir e os botões na HUD).
- Os três NPCs já estão carregados na configuração do servidor em `server/npc/custom/myth/skill_quests.conf`.
