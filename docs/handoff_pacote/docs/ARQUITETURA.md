# Arquitetura do cliente (regras das camadas)

```
rAthena (TCP)
   │  pacotes binários do RO
   ▼
Source/MythOfThor/Network/   CAMADA 1: rede. URagnarokNetworkSubsystem lê o socket, decodifica o pacote e dispara um delegate
   │                         (FOnLoginResult, FOnCharacterList...). Não conhece UI, mundo nem sprites.
   ▼
Config/*.json + Core/        CAMADA 2: dados. Tabelas geradas por tools/ a partir do rAthena (item_db, skill_db), do GRF do
   │                         cliente e do roBrowser. FMythRoClientTables (Core/) é a porta de acesso: o servidor manda só IDs.
   ▼
World/ Player/ Sprites/ UI/  CAMADA 3: apresentação e jogabilidade. Escutam os delegates da rede, consultam as tabelas e
                             desenham (mover, animar, efeito, janela). Pedidos ao servidor saem pelo subsistema de rede.
```

## Regras

1. **Rede só emite eventos.** `Network/` não inclui nada de `UI/`, `World/`, `Player/` ou `Sprites/`. Se precisar de um nome ou
   dado de item/skill, usa `Core/MythRoClientTables.h`.
2. **Dados só vêm de `Config/`.** Nenhum valor de item, skill, monstro ou fórmula fica fixo no código; se faltar, gera-se a
   tabela pelo script em `tools/` (e o script lê a fonte: rAthena, GRF ou roBrowser).
3. **O servidor decide.** O cliente só envia pedidos (andar, usar skill, atacar) e mostra a resposta. Não recalcula dano, drop,
   alcance nem requisitos que o rAthena já valida.
4. **Camada 3 não fala socket.** Janelas, mundo e controle chamam métodos do subsistema de rede (`UseSkill`, `Move`...) e
   escutam seus delegates; não abrem conexão nem leem buffer.
5. **Efeitos de skill são dirigidos por dados.** `Config/skill_effects_full.json` (tools/gen_skill_effects_ro.py) diz qual
   efeito tocar em cada momento; o código só sabe desenhar os tipos de parte (STR, SPR, 3D, TRAIL, QuadHorn...).

## Onde está cada coisa

| Pasta | Conteúdo |
|---|---|
| `Network/` | subsistema de sockets, pacotes por assunto (guilda, pet, homúnculo, listas de seleção, venda) |
| `Core/` | matemática de direções, `FMythRoClientTables` (acesso às tabelas do cliente) |
| `World/` | `MythNetWorld` (cria/remove entidades a partir dos eventos), monstros, NPCs, itens no chão |
| `Player/` | controle, movimento, rota (A*), comandos de teste |
| `Sprites/` | sprites do RO (SPR/ACT), efeitos de skill, falcão, aura |
| `UI/` | HUD e janelas no estilo do cliente LATAM |

## Dívidas conhecidas
- `Core/MythRoClientTables.cpp` ainda inclui `Sprites/` e `UI/MythRoUITextures.h` para carregar ícones (só o `.h` é limpo, que é
  o que a rede inclui). Separar o carregamento de texturas da consulta de nomes resolveria.
- Um módulo C++ separado para a rede só vale se o tempo de compilação ou o tamanho do código passar a atrapalhar.
