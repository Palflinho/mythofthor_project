status: feito

# Resposta 008: Borda Rosa/Magenta Eliminada e Reimportação dos Mapas

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefas 006 e 007

---

## 1. Causa Raiz e Solução da Borda Magenta

### O Problema
Nos arquivos `.bmp` originais do Ragnarok Online, a cor `#FF00FF` (magenta puro) é utilizada como convenção de transparência (chroma keying). Quando essas texturas eram importadas na Unreal Engine:
1. O corte da transparência era feito no material via cálculo de distância matemática de cor do pixel.
2. Porém, o **filtro bilinear** e a **geração de mipmaps** da GPU realizam interpolação ponderada (filtro de caixa) antes do teste de corte.
3. Isso gerava pixels intermediários nas bordas onde a folha verde (ex: `#52661E`) se misturava com o magenta (`#FF00FF`), resultando em tons de rosa/púrpura que não eram cortados pelo shader nem ficavam verdes, criando a borda brilhante visível em árvores e canteiros.

### A Correção no Pré-Processamento (`tools/ue_import_map.py`)
Implementamos a função `process_texture_magenta(src_path, dst_png_path, passes=4)`:
1. **Identificação Robusta de Chroma Key:**
   - Pixels com distância euclidiana ao `#FF00FF` menor que 40 (ou `R > 200, G < 60, B > 200`) têm seu canal alfa definido como `0`. Pixels opacos têm `A = 255`.
2. **Dilatação de Cor em 4 Passadas (Edge Color Padding):**
   - Para evitar que o bilinear interpole com a cor magenta de fundo, executamos 4 passadas de dilatação vetorial (via NumPy). Em cada passada, pixels transparentes adjacentes a pixels opacos recebem a média de cor RGB dos seus vizinhos de 8 conexões opacos, mantendo `A = 0`.
   - Dessa forma, o fundo imediatamente vizinho à folha passa a ter a mesma cor da folha (`#52661E`), eliminando 100% da contaminação rosa na amostragem subpixel e nos mips.
3. **Exportação Nativa RGBA e Material `M_RO_Masked`:**
   - A textura é gravada como PNG 32-bit (RGBA).
   - O material mestre `M_RO_Masked` foi simplificado para conectar diretamente a saída do canal alfa `TextureSample.A` no pino `MP_OPACITY_MASK`, com `OpacityMaskClipValue = 0.3333`.

---

## 2. Reimportação dos 7 Mapas (Exit Code 0)

Todos os mapas foram reimportados sequencialmente através do script [`tools/reimport_maps.ps1`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tools/reimport_maps.ps1), mantendo o `coord/build.lock` ativo durante a operação e liberando ao final:

| Mapa | BGM Associado | Posição Inicial / Parâmetros | Status |
| :--- | :--- | :--- | :--- |
| `prontera` | `08.mp3` | `--start-cell 156,191` | **Sucesso (Code 0)** |
| `prt_fild05` | `12.mp3` | Padrão | **Sucesso (Code 0)** |
| `prt_fild06` | `12.mp3` | Padrão | **Sucesso (Code 0)** |
| `prt_fild08` | `12.mp3` | Padrão | **Sucesso (Code 0)** |
| `prt_in` | `210.mp3` | Padrão (alinhamento GAT 2:1 preservado) | **Sucesso (Code 0)** |
| `prt_church` | `10.mp3` | Padrão (alinhamento GAT 2:1 preservado) | **Sucesso (Code 0)** |
| `prt_castle` | `09.mp3` | Padrão (alinhamento GAT 2:1 preservado) | **Sucesso (Code 0)** |

---

## 3. Validação Visual (Antes x Depois)

- **Antes (`Saved/Screenshots/WindowsEditor/c_01_praca.png`):** As copas das árvores da praça e os canteiros de flores ao redor da fonte exibiam forte contorno rosa neon.
- **Depois (`Saved/Screenshots/WindowsEditor/m_01_arvores.png`):** As flores e folhagens agora têm corte suave, natural e totalmente livre de resíduos rosas.

---

## 4. Observação sobre Execução de Jogo / Prints

Anotado e registrado:
- Para capturas automatizadas com `-game`, usar `UnrealEditor.exe` (e não `UnrealEditor-Cmd.exe`), garantindo que o ciclo de vida do processo feche adequadamente sem reter handles nas DLLs de jogo.

---

## 5. Próximos Passos

Os arquivos `tools/ue_import_map.py` e `tools/reimport_maps.ps1` estão prontos e sendo commitados. Em seguida, iniciaremos o desenho técnico da **Fase 7.2** em `docs/sistemas/06_quests_habilidades.md` para as habilidades de quest (Quest Skills).
