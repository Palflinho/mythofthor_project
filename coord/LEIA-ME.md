# Coordenação automática Claude ⇄ Antigravity

Os dois agentes trocam tarefas por arquivos nesta pasta, sem o usuário precisar repassar mensagens.

## Pastas

| Pasta | Quem escreve | Quem lê |
|---|---|---|
| `coord/para_antigravity/` | Claude | Antigravity |
| `coord/para_claude/` | Antigravity | Claude |

Cada mensagem é um arquivo `NNN_assunto.md` (número crescente, 3 dígitos, por pasta). A primeira linha é sempre:

```
status: pendente
```

O destinatário troca para `status: em_andamento` quando começa e `status: feito` quando termina, e responde com um
arquivo novo na pasta do outro (`status: pendente`), citando o número da tarefa (ex.: "Resposta à 001").
Dúvidas também vão como arquivo novo; quem pergunta não para, segue com outra parte enquanto espera.

## Esperar mensagem (cada agente roda em loop)

```powershell
powershell -ExecutionPolicy Bypass -File coord/esperar.ps1 -Para antigravity
```

O script fica parado até aparecer um arquivo com `status: pendente` na pasta do agente, imprime o conteúdo e sai.
Sem novidade em 25 min, sai com "NADA NOVO" (código 1): é só rodar de novo. Fluxo de cada agente:

1. rodar `esperar.ps1`;
2. ler a mensagem, marcar `em_andamento`, fazer o trabalho, commitar (local, sem push);
3. marcar `feito`, escrever a resposta na pasta do outro;
4. voltar ao passo 1.

## Build (um de cada vez)

Use sempre `tools/build_watch.ps1`. Ele cria `coord/build.lock` (quem e quando), recusa começar se o lock existir
ou se outro UnrealBuildTool estiver rodando, apaga o lock no fim e mata o build se ficar 5 min sem progresso.
Não edite arquivos de `Source/` que o outro agente declarou em uma tarefa `em_andamento`.

## Regras que continuam valendo

- O rAthena faz as regras do jogo; o cliente só mostra. IDs e offsets de pacote conferidos no código do rAthena.
- Commits locais, sem push.
- Textos para o usuário em português.
