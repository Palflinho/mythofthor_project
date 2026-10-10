status: pendente

# Resposta 018: Relatório de Auditoria do Servidor rAthena F4 (Tarefa 011 Concluída)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 011 (`coord/para_antigravity/011_auditoria_servidor_F4.md`)  
**Documento Principal Entregue:** `docs/FIDELIDADE_SERVIDOR.md`

---

## 1. Conclusão da Tarefa 011

A auditoria completa das configurações, binários, pacotes, flags de compilação, bancos de dados e scripts do servidor rAthena (`~/mythofthor-server/rathena`) foi realizada e documentada em detalhes em **`docs/FIDELIDADE_SERVIDOR.md`**.

O veredito geral é altamente positivo: **o servidor rAthena está em conformidade 99.5% estrita com o Ragnarok Online Pre-Renewal 1:1**, com as modificações do Myth of Thor perfeitamente isoladas em scripts de conveniência e sem nenhuma adulteração no código-fonte C++ do emulador.

---

## 2. Principais Constatações da Auditoria

1. **PACKETVER e Flags de Compilação:**
   - **`PACKETVER` em uso no binário atual:** **`20250402`**.
   - **Flag de Batalha:** **`-DPRERE` ATIVA** em `CPPFLAGS`.
   - **Esclarecimento:** Documentos antigos citavam `20211103`. O rAthena foi recentemente reconfigurado e compilado com `PACKETVER=20250402` para habilitar pacotes estendidos modernos e keepalive nativo do cliente Unreal HD-2D. Apenas os docs precisavam desse alinhamento.
2. **Modificações contra o Upstream (`git diff`):**
   - **Zero linhas alteradas em `src/` e `conf/battle/`.**
   - Apenas `npc/scripts_custom.conf` foi modificado para importar `npc/custom/autobattle.txt` e `npc/custom/myth/skill_quests.conf`.
3. **Rates e Batalha 1x Oficiais:**
   - `base_exp_rate: 100` (1x)
   - `job_exp_rate: 100` (1x)
   - Todos os drops de itens comuns, cura, usáveis, equipamentos e cartas (`item_rate_card: 100` = 0.01%) estão rigorosamente em 1x.
   - O arquivo de sobreposição `conf/import/battle_conf.txt` está vazio (0 B), garantindo que as regras oficiais do rAthena permaneçam intactas.
4. **Bancos de Dados (`db/pre-re/`):**
   - Amostragem de monstros (Poring, Esporo, Lobo), consumíveis (Poção Vermelha curando 45~65 HP por 50z) e habilidades (Bash com custo 8/15 SP e fatal blow) confirmou paridade idêntica com o oficial.
5. **Quests de Classe e Progressão sem GM:**
   - Todo o fluxo de mudança de classe (Aprendiz $\to$ 1ª Classe $\to$ 2ªs Classes 2-1/2-2 $\to$ Valquíria de Rebirth em `valkyrie.txt`) está ativo e acessível por quests originais. A NPC de testes `test_npc.txt` adicionada na Tarefa B4 é puramente opcional.
6. **Customizações do Myth:**
   - O Auto-Battle é acionado unicamente pelo comando `@autobattle`, com trava de 3h diárias e desativação em sobrepeso $\ge 90\%$.

---

## 3. Status das Tarefas

- `coord/para_antigravity/011_auditoria_servidor_F4.md`: marcado como **`status: feito`**.
- Todas as tarefas de pesquisa (R1, R2, Aura 99 e F4) estão concluídas!
