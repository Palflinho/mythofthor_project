# Índice Geral das Especificações de Referência (R1)

> **Projeto:** Myth of Thor (Fidelidade ao Ragnarok Online na Unreal Engine 5 HD-2D)  
> **Objetivo:** Fornecer especificações completas de comportamento, regras, números exatos, fórmulas e tabelas extraídas diretamente dos clientes de código aberto de referência (**Open-Midgard [OM]**, **roBrowserLegacy [RB]** e **Korangar [KO]**), para que o Claude e a equipe de desenvolvimento possam implementar a mecânica e a interface do cliente de forma 100% fiel ao executável original.

---

## 1. Índice dos Documentos

| Arquivo | Título do Sistema | Resumo (1 Linha) | Referências Principais |
|---|---|---|---|
| [**01_movimento.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/01_movimento.md) | **Movimento e Pathfinding** | Algoritmo de caminho A* modificado no cliente, custos 10/14, corte de quinas, interpolação temporal e correção de posição com o servidor. | OM `PathFinder.cpp`, `GameActor.cpp`; RB `EntityWalk.js` |
| [**02_camera.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/02_camera.md) | **Câmera e Controles** | Ângulo de inclinação padrão de -35° (limites -15° a -65°), zoom padrão 150 (30–250), sensibilidade de rotação de 0.2°/px e reset. | OM `View.cpp`; RB `Camera.js` |
| [**03_atores_animacao.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/03_atores_animacao.md) | **Atores e Animação** | Ciclos de quadros por .act e ASPD, ordenação de 5 camadas do personagem por direção, doridori ocioso e sombra redonda. | OM `GameActor.cpp`, `3dActor.cpp`; RB `EntityRender.js` |
| [**04_clique_alvo_combate.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/04_clique_alvo_combate.md) | **Clique, Alvo e Combate** | Hierarquia rígida de raycast (UI > NPC > Mob > Item > Chão), alcance físico, aproximação em falha de distância e cursores. | OM `GameMode.cpp`, `GameModeSelectTarget.cpp`; RB `Cursor.js` |
| [**05_efeitos_do_executavel.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/05_efeitos_do_executavel.md) | **Efeitos Procedurais** | Efeitos gerados em código sem .str: anéis e cilindros de portais, arco parabólico de números de dano, Miss, críticos, cura e aura 99. | OM `RagEffect.cpp`, `MsgEffect.cpp`; RB `Renderer/Effects/*` |
| [**06_janelas_basicas.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/06_janelas_basicas.md) | **Janelas Básicas de UI** | Posições, dimensões, atalhos e interações completas da suíte clássica de 2008 (BasicInfo, Item, Equip, Status, Skill, Chat, Npc, Storage). | OM `ui/UI*Wnd.cpp`; RB `UI/Components/*` |
| [**07_sons.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/07_sons.md) | **Efeitos Sonoros e BGM** | Mapeamento de BGM contínua via `mp3nametable.txt`, disparos de WAV pelo .act, resolução de armas e armaduras, e atenuação espacial (25 células). | OM `Audio.cpp`, `Audio.h`; RB `SoundManager.js`, `WeaponSoundTable.js` |
| [**08_ui_moderna_rb.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/08_ui_moderna_rb.md) | **Sistemas de UI Moderna** | Inventário em abas com travas de segurança, navegador geográfico `/navi`, janela e rastreador de missões (Alt+U), conquistas, banco e RODEX. | RB `UI/Components/*` (InventoryV3, Navigation, Quest, Rodex, Bank) |
| [**09_render_korangar.md**](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/09_render_korangar.md) | **Render e Iluminação HD-2D** | Geração de normais cilíndricas para dar volume 3D aos sprites 2D, prevenção de Z-fighting via offsets, e integração com o Lumen do UE5. | KO `shaders/passes/forward/entity.slang`, `model.slang` |

---

## 2. O que Ainda Não Foi Encontrado / Pontos em Aberto

Durante o mapeamento e leitura dos repositórios de referência, alguns detalhes extremamente específicos do executável original da Gravity requerem verificação futura ou decisão de projeto:

1. **Curva de Inércia de Montarias (Peco Peco / Grifo):**
   - Nos clientes abertos pesquisados, o ganho de velocidade de montaria é aplicado de forma puramente linear e instantânea na variável `walkSpeed` do ator (`GameActor.cpp`), sem curvas de aceleração ou frenagem aparentes. Caso o cliente oficial tenha algum micro-delay de aceleração ao montar, ele não está modelado nos clientes abertos.
2. **Offsets Exatos de Chapéus no Formato Antigo:**
   - No cliente clássico, cada frame de cabeça alinha com o corpo usando o ponto de ancoragem nativo do `.act`. Entretanto, certas combinações de cabelos volumosos e elmos do Renewal dependem de um arquivo de exceções (`headgear_offset.lub` ou similar no cliente oficial da Gravity). Para o Myth of Thor, o sistema de âncoras do `.act` é suficiente para 99% dos casos da Fase 1 (classes 1-1, 1-2 e transclasses).
3. **Desvio de Colisão Dinâmica entre Jogadores:**
   - Quando o jogador tenta passar por uma célula ocupada por outro jogador em cidades ou mapas que proíbem sobreposição de atores, o cliente envia a rota normal e é o servidor rAthena que responde rejeitando o passo ou recalculando. O cliente em si não faz cálculo prévio de evitar unidades móveis no A* local para não sobrecarregar a CPU.

---

## 3. Próximos Passos Recomendados

1. **Implementação de Movimento e Câmera no Claude:**
   - O Claude pode utilizar imediatamente as especificações `01_movimento.md` e `02_camera.md` para finalizar o movimento sem o efeito "anda e volta" do WASD e calibrar a inclinação e rotação exata da câmera em Unreal.
2. **Material Master HD-2D:**
   - Aplicar a fórmula de normal cilíndrica de `09_render_korangar.md` no material de sprite do jogador e monstros para desbloquear a iluminação volumétrica do Lumen.
3. **Painéis de UI:**
   - Utilizar as especificações `06_janelas_basicas.md` e `08_ui_moderna_rb.md` para compor as janelas da HUD com texturas e comportamentos fiéis ao RO.
