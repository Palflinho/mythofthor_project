# Relatório da noite (2026-10-12 → 13)

Trabalho feito sem confirmação, como combinado. Cada item tem o commit e foi conferido em jogo com a conta `mythclaude`
(o personagem ClaudeTester foi devolvido ao estado de antes: Caçador 99/10, Gladius). O que ficou pendente está no fim.

## 1. Documentos

- `HUD_REVISAO_COMPLETA.md` e `HANDOFF_OUTRO_CLAUDE.md` atualizados com o estado real do código (`0f213b1`, `5a554d8`, `9c007a9`).
- `PRERE_X_RENOVACAO.md`: comparação pré-renovação × renovação com números do nosso rAthena e a sua decisão (`d660f60`).

## 2. Servidor em Renovação, até a transclasse (`c94bd6f`, `e1c9255`)

- rAthena recompilado com `--enable-prere=no` (PACKETVER 20250402). Banco da Renovação carregado: 2.675 monstros,
  12.982 equipamentos, 23.677 NPCs.
- Transclasse **125/70** (testado: Atirador de Elite chega a 125 e para); 2ª classe 99/50; expandidas finais (Justiceiro, Ninja,
  Mestre Taekwon, Espiritualista, Super Aprendiz) 125; EXP 99→125 pela curva oficial da Renovação.
- Atributos até **110** (testado: em 110 o custo vira 0; acima de 100 o ponto custa mais, como na Renovação).
- Bloqueado: NPCs de 3ª classe, Kagerou/Oboro, Rebelde, Super Aprendiz Expandido e Doram (também na criação de personagem).
- Tudo reproduzível por `server/renovacao/aplicar.sh` (os arquivos do rAthena ficam no WSL).
- Cliente: Status mostra MATK como "atributo + equipamento", ASPD como o cliente original (147 em vez de 530) e o custo do
  próximo ponto vem do servidor.
- **Sprites de monstros** (`9284e27`): a tabela usava o nome do servidor (AegisName) e 1.663 dos 2.675 monstros ficavam sem
  sprite (até o Chonchon, que no cliente se chama `chocho`). Agora usa a tabela do próprio cliente (`jobname.lub`): faltam só
  265 (guardiões e Emperium da GdE, baús).

## 3. Janelas no nível do LATAM

- **Equipamento** (`595b1a9`): 4 abas (Geral, Especial, Título, Fonte de dano), encaixes com o nome em até 2 linhas, **prévia do
  personagem** desenhada direto dos sprites (muda na hora ao trocar chapéu/arma), botão redondo "off" do falcão/Peco e rodapé
  com "Mostrar equip." (o servidor guarda), Status, Troca rápida e **Tirar tudo**.
- **Navegação** (`1e8a73b`): filtro Todos/Mapas/Monstros, "Resultado[n]", mapa com a lista de **monstros do mapa** (nome e
  tamanho), "Procurar" mostra a rota pelos portais, "Enviar" põe `/navi <mapa>` no chat, "Mover" anda sozinho.

## 4. Correções de jogo (P0 da auditoria)

- **NPCs que pedem número ou texto não travam mais** (`a43bf70`): caixa de entrada abaixo da fala, OK/Enter manda ao servidor,
  Cancelar encerra. Testado com o Assistente de Testes (nível por número e um texto). As falas (`mes`) agora saem uma por linha.
- **Munição** (`a91f563`): o cliente achava que toda pilha de flechas estava equipada (lia o campo errado). Agora mostra a
  flecha que o servidor equipou de verdade; "sem flechas" aparece no chat em vermelho com o texto do cliente LATAM
  ("Você deve equipar flechas antes."); Memo (Portal) mostra o resultado.

## Pendências e decisões que deixei para você

- (preencher ao fim da noite)
