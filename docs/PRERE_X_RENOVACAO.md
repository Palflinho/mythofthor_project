# Pré-renovação × Renovação no servidor (2026-10-12)

Para decidir qual conjunto de regras o rAthena usa. **O cliente não muda nos dois casos:** a HUD, as janelas e as traduções vêm do
RO LATAM (`C:\Gravity\Ragnarok`, atualizado em 2026-10-07, o cliente oficial no ar). "Pré-renovação" ou "Renovação" é uma opção de
compilação do **servidor** (`./configure --enable-prere=yes|no`) que troca as fórmulas e o banco de dados inteiro.

**Hoje:** o servidor está compilado em **pré-renovação** (`config.log`: `--enable-packetver=20250402 --enable-prere=yes`), com o rAthena `d4b8e7b` (2026-10-06).

## 1. Regras (o que o `src/config/renewal.hpp` liga no modo Renovação)

| Assunto | Pré-renovação (hoje) | Renovação |
|---|---|---|
| Conjuração | DEX reduz o tempo; **150 de DEX = conjuração instantânea** | Tempo variável cai com DEX × 2 + INT (some em 265) e **20% é fixo**, só reduzido por item/buff |
| ASPD | Fórmula antiga, teto 190 | AGI pesa mais, **escudo penaliza**, bônus de item/skill que não acumulam |
| Dano | ATK e MATK clássicos; DEF reduz em porcentagem direta | ATK = status + arma, MATK com mínimo/máximo, DEF em fórmula de redução decrescente; nível do personagem aumenta o dano de algumas skills |
| EXP e drop | Iguais para qualquer nível | Mudam conforme a diferença de nível entre jogador e monstro |
| Pontos de atributo | Tabela antiga | Tabela da Renovação |
| Skills | Versão clássica (ex.: Asura, Sônico, Cura) | Versões rebalanceadas |
| Nível máximo até a transclasse | 99 / 70 | 99 / 70 (o resto da progressão é das 3ªs classes) |

## 2. Conteúdo (contagem no banco do rAthena `d4b8e7b`)

| | Pré-renovação | Renovação |
|---|---|---|
| Monstros | 1.004 (só 25 acima do nível 99) | 2.671 (**1.458 acima do nível 99**) |
| Equipamentos | 2.017 | 12.982 |
| Consumíveis | 1.905 | 6.415 |
| Itens diversos | 2.247 | 9.959 |
| Missões | 2.879 | 4.821 |
| Instâncias | 4 | 78 (48 scripts) |
| Scripts de NPC | cidades, empregos, Kafras, missões | tudo isso + campos de batalha, guilda 3, instâncias, episódios recentes |

## 3. O que pesa na decisão

- **Mais da metade do conteúdo da Renovação é para nível acima de 99.** Com o jogo limitado à transclasse (99/70), esses mapas,
  monstros e equipamentos ficam fortes demais ou sem uso. A Renovação "até a transclasse" nunca existiu oficialmente: a Renovação
  chegou junto com as 3ªs classes e foi balanceada para elas.
- **A pré-renovação é balanceada para o 99.** Tudo nela foi feito para personagens até a transclasse.
- **Os sistemas modernos funcionam nos dois modos**, porque dependem da versão do pacote (PACKETVER), não das regras: Rodex, conquistas,
  presença diária (`attendance`), navegação, janela de missões nova, banco, troca rápida. O pré-renovação do rAthena tem `achievement_db`
  e `attendance` próprios.
- **Custo de trocar para Renovação:** recompilar o servidor, trocar o banco (`db/re`, `npc/re`), revisar `docs/FIDELIDADE.md` e
  `FIDELIDADE_SERVIDOR.md` (escritos para pré-renovação), resetar os atributos dos personagens de teste e revisar a janela de Status
  (ATK/MATK/DEF no formato da Renovação). O cliente quase não muda, porque só mostra o que o servidor manda.
- **Dá para voltar atrás:** é uma opção de compilação; os dados dos personagens continuam no banco.

## 4. Recomendação

Pré-renovação nas **regras e no conteúdo**, com o **cliente e os sistemas modernos do LATAM** (HUD, Rodex, conquistas, navegação,
missões, banco). É a combinação que fecha com "até a transclasse": tudo o que é atual e funciona com 99/70, sem metade do mundo
balanceado para quem não existe no jogo.

Escolher Renovação faz sentido se a ideia for, no futuro, abrir as 3ªs classes ou usar os mapas e episódios novos com outro nivelamento.

## 5. Decisão do usuário (2026-10-12): Renovação, até a transclasse "por enquanto"

| Regra | Valor |
|---|---|
| Modo do servidor | Renovação (`--enable-prere=no`, PACKETVER 20250402) |
| Transclasse | nível base **125**, classe 70 (EXP 99→125 pela curva oficial da Renovação, a das 3ªs classes) |
| 2ª classe | 99 / 50 (renascer continua exigindo 99/50) |
| Classes expandidas | liberadas: Justiceiro, Ninja, Taekwon → Mestre Taekwon / Espiritualista, Super Aprendiz; as finais vão a 125 |
| Atributos | máximo **110** (bebês ficam no padrão 80) |
| Bloqueado | 3ª e 4ª classes, Kagerou/Oboro, Rebelde, Super Aprendiz Expandido, Doram (criação e NPCs de mudança) |
| Aura | continua no nível 99, como no cliente oficial para essas classes |

### Como foi aplicado

Tudo em `server/renovacao/` (no repositório; o rAthena fica no WSL):
- `aplicar.sh`: grava `conf/import/battle_conf.txt` (110), gera `db/import/job_stats.yml` (`gen_job_stats.sh`), põe
  `allowed_job_flag: 1` no `char_conf` do docker e importa `escopo_classes.conf` (`delnpc` dos NPCs de 3ª classe etc.).
  Rodar como root: `wsl.exe -d Ubuntu -u root -- bash "/mnt/e/Documentos/Repositórios/mythofthor_project/server/renovacao/aplicar.sh"`.
- Recompilar: no `server/`, `docker compose --profile build run --rm builder sh -c './configure --enable-packetver=20250402 --enable-prere=no && make clean server'`
  e reiniciar `mythofthor-login mythofthor-char mythofthor-map`. O `server/docker-compose.yml` já tem `--enable-prere=no`.
- Cliente: a janela de Status mostra MATK como "atributo + equipamento" e o custo do próximo ponto vem do servidor.
- Mensagens do servidor em português (2026-10-13): `aplicar.sh` põe `#define LANG_ENABLE 0x80` no `src/custom/defines_pre.hpp`,
  converte `conf/msg_conf/map_msg_por.conf` para UTF-8 e carrega `npc/custom/myth/idioma.txt` (cada conta vira `@langtype por`
  ao entrar). Depois de mudar o define é preciso recompilar `src/common/msg_conf.cpp` (um `touch` nele antes do `make server`).
