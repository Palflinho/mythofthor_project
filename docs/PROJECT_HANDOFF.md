# Myth of Thor — Project Handoff & Continuity Brief

> **Repositório:** `https://github.com/Palflinho/mythofthor_project` (Branch: `main`)  
> **Caminho Local Padrão:** `D:\Repositórios\mythofthor_project`  
> **Assets Extraídos:** `D:\Ragnarok_Assets` (ou extraídos do `data.grf` via `tools/`)  
> **Stack:** Unreal Engine 5.4+ (C++ / Paper2D / Enhanced Input / Lumen) + rAthena (C/C++ nativo TCP via Docker/WSL).

---

## 1. Visão do Projeto & Regras Inegociáveis
1. **Fidelidade RO Clássico:** Fórmulas originais (ASPD `(200 - ASPD)/50 s`, Cast Time, Flee, Hit, Def).
2. **Cap Progressivo (Fase 1: Cap 30):** Novice, classes 1-1 (Espadachim, Mago, Arqueiro, Gatuno, Noviço, Mercador). Doram e 4ª classe estritamente proibidos. WoE desativada na Fase 1.
3. **Estética HD-2D:** Sprites 2D pixel-art sem filtro bilinear (`Nearest`, `2D Pixels Unfiltered`) em cenários 3D com iluminação Lumen dinâmica, sombras e ciclo Dia/Noite.
4. **Auto-Battle com Limites:** Vigor diário de 3 horas, trava de sobrepeso em 90%, penalidade de morte de 5% Base EXP, temporizador de 2 min caído e retorno a Prontera com bot pausado.

---

## 2. Divisão de Trabalho entre IAs
- **Claude:** Gameplay UE5, Blueprints, Enhanced Input, Câmera (45° fixa / Livre 360°), Paper2D Billboard Sprites, UMG/Canvas HUD (Círculo de alvo, barras de HP/SP/EXP, números de dano flutuante), Ciclo Dia/Noite.
- **Antigravity:** Subsystem de Rede C++ (`URagnarokNetworkSubsystem`), Protocolo nativo de pacotes do rAthena (TCP), Docker/WSL Backend, Scripts de conversão de assets (`compile_map_to_obj.py`, `extract_and_convert_sprites.py`), Automação e Testes.

---

## 3. Estado Atual do Código (Último Commit no GitHub)
- **Rede (`Source/MythOfThor/Network`):**
  - Subsystem assíncrono conectando via sockets TCP brutos.
  - Pacotes implementados e validados: `CA_LOGIN` (0x02B0), `CH_ENTER` (0x0065), `CZ_REQUEST_ACT` (0x0360 - Ataque normal), `CZ_USE_SKILL` (0x0113 - Habilidade com alvo).
  - 11 testes unitários de pacotes passando em `tests/ragnarok_net_test.py`.
- **Combate & Controles (`Source/MythOfThor/Player` & `Source/MythOfThor/UI`):**
  - Mouse Point & Click + WASD simultâneos com transição suave.
  - Seleção de monstro via `ClickBox` e anel de seleção desenhado dinamicamente no Canvas HUD sem depender de Decals/Materials.
  - Disparo de ataque conectado a `SendActionRequest` (0x0360) e feedback imediato com `UMythCombatText::ShowDamage` (números normais e críticos).
- **Mundo & Áudio (`Source/MythOfThor/World`):**
  - `MythDayNightActor` com ciclo solar/lunar e iluminação dinâmica.
  - `MythNightAudioComponent` para trilhas e efeitos sonoros ambientais.
- **Assets Gerados (`tools/`):**
  - Mapas completos compilados para OBJ/MTL: `prontera_full.obj` e `new_zone01_full.obj`.
  - Mais de 45 spritesheets em PNG RGBA transparente extraídos de `data.grf`.

---

## 4. Próximos Passos Imediatos
1. **Barra de Vida do Monstro:** Exibir nome e barra fina de HP sobre o monstro selecionado via Canvas HUD.
2. **Reação a Dano e Morte:** Adicionar animação de `Hit` e rotina de `Kill` (limpando o target e destruindo o ator após o fade).
3. **Importação do Mapa de Prontera na UE5:** Executar `tools/ue_import_map.py` para carregar `prontera_full.obj`, aplicar colisão complexa como simples e gerar o NavMesh Bounds.
4. **Loop Servidor-Cliente:** Subir o container Docker do rAthena e testar movimentação e ataque autenticado no servidor.
