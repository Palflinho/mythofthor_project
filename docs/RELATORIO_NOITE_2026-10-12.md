# Relatório da noite (2026-10-12 → 13)

Trabalho feito sem confirmação, como combinado. Cada item tem o commit; o que ficou pendente está no fim.

## 1. Documentos

- `HUD_REVISAO_COMPLETA.md` e `HANDOFF_OUTRO_CLAUDE.md` atualizados com o estado real do código (`0f213b1`, `5a554d8`).
- `PRERE_X_RENOVACAO.md`: comparação pré-renovação × renovação com números do nosso rAthena (`d660f60`) e a decisão.

## 2. Servidor em Renovação, até a transclasse (`c94bd6f`, `e1c9255`)

- rAthena recompilado com `--enable-prere=no` (PACKETVER 20250402). Banco da Renovação carregado: 2.675 monstros,
  12.982 equipamentos, 23.677 NPCs.
- Transclasse 125/70; 2ª classe 99/50; expandidas finais (Justiceiro, Ninja, Mestre Taekwon, Espiritualista, Super Aprendiz) 125;
  EXP 99→125 pela curva oficial da Renovação. Atributos até 110.
- Bloqueado: NPCs de 3ª classe, Kagerou/Oboro, Rebelde, Super Aprendiz Expandido e Doram (inclusive na criação de personagem).
- Tudo reproduzível por `server/renovacao/aplicar.sh` (os arquivos do rAthena ficam no WSL).
- Cliente: janela de Status mostra MATK como "atributo + equipamento" (formato da Renovação) e o custo do próximo ponto vem do servidor.

## 3. Janela de Equipamento no nível do LATAM (print 08)

- 4 abas (Geral, Especial, Título, Fonte de dano), 280 de largura, encaixes em linhas de 26 com o nome em até 2 linhas.
- **Prévia do personagem** no centro, desenhada direto dos .spr/.act (mesmas camadas do mundo), atualiza ao trocar o visual.
- Rodapé: "Mostrar equip." (CZ_CONFIG, o servidor guarda), botões de atributos, troca rápida e **tirar tudo** (CZ_REQ_TAKEOFF_EQUIP_ALL).
- Botão redondo "off" para soltar o falcão / descer do Peco, como no print.

## Pendências e decisões que deixei para você

- (preencher ao fim da noite)
