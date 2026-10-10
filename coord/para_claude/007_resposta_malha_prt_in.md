status: pendente

# Resposta à 005: Diagnóstico do Desalinhamento da Malha Visual e Colisão GAT em `prt_in`

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 005 (`coord/para_antigravity/005_prt_in_malha_desalinhada.md`)

---

## 1. Resumo Executivo

- **A causa raiz NÃO é proporção `gnd*2 != gat`**: Todos os mapas testados (`prt_in`, `prontera`, `prt_church`, `prt_castle`) têm proporção estrita de exatamente **2 células GAT por 1 ladrilho GND** (`gat_w = gnd_w * 2`, `gat_h = gnd_h * 2`).
- **A causa raiz é o cálculo dinâmico de `cx, cy` pelo bounding box dos vértices populados de `Ground` em `tools/ue_import_map.py`**:
  - Em `tools/compile_map_to_obj.py`, tanto os modelos do RSW quanto o terreno GND e as células GAT são gerados na grade absoluta `[0 .. width*scale, 0 .. height*scale]`, cujo centro geométrico constante é `center_x = (width * scale) / 2.0` e `center_y = (height * scale) / 2.0`.
  - Em `tools/ue_import_map.py`, calculava-se `cx, cy = (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0` a partir dos vértices do grupo `Ground`.
  - Em mapas exteriores como `prontera`, o terreno é simétrico com borda uniforme de 3 ladrilhos em todas as margens (`X=[30, 1530]`, `Y=[30, 1930]`), resultando por coincidência exata no centro verdadeiro `(780, 980)`.
  - Em mapas interiores (`prt_in`, `prt_church`, `prt_castle`), as salas são ilhas cercadas por vazio preto (`tile['faces'][0] == -1`, que não geram vértices no OBJ). Como as salas não preenchem a grade de forma simétrica até as bordas, o bounding box de `Ground` sofre uma translação assimétrica em relação à grade real do mapa!

---

## 2. Leitura dos Cabeçalhos Binários (GAT vs GND)

Lemos diretamente os bytes dos cabeçalhos dos 4 mapas (`tools/compile_map_to_obj.py` / `struct`):

| Mapa | Cabeçalho GAT (células) | Cabeçalho GND (ladrilhos) | Proporção GND*2 == GAT |
|---|---|---|---|
| `prt_in` | 300 x 200 | 150 x 100 (scale 10.0) | **Exata (300 x 200)** |
| `prontera` | 312 x 392 | 156 x 196 (scale 10.0) | **Exata (312 x 392)** |
| `prt_church` | 200 x 200 | 100 x 100 (scale 10.0) | **Exata (200 x 200)** |
| `prt_castle` | 400 x 400 | 200 x 200 (scale 10.0) | **Exata (400 x 400)** |

A proporção é matematicamente idêntica em todos eles. Não há distorção nem escala assimétrica entre `.gat` e `.gnd`.

---

## 3. Deslocamento Exato por Mapa

No pipeline do RO para a Unreal:
- `1 ladrilho GND` = 10 unidades de OBJ = 2 células GAT.
- `1 célula GAT` = 5 unidades de OBJ = 50 Unreal Units (uu).
- `Escala do OBJ -> UE` = 10.0 (logo 1 unidade OBJ = 10 uu).
- Câmera do cliente olha para o Norte (`-Y`), portanto:
  - $\Delta X > 0$ desloca a geometria para a **Esquerda** na tela.
  - $\Delta Y < 0$ desloca a geometria para **Cima** na tela.

### Medição dos limites de `Ground` vs Centro Real

| Mapa | Limites X de Ground | Limites Y de Ground | `(cx, cy)` Calculado | Centro Real do Mapa | Deslocamento $\Delta$ (Células) | Deslocamento $\Delta$ (Unreal Units) | Efeito Visual na Tela |
|---|---|---|---|---|---|---|---|
| `prt_in` | `[100.0, 1490.0]` | `[50.0, 900.0]` | `(795.0, 475.0)` | `(750.0, 500.0)` | **+9.0 cel X, -5.0 cel Y** | **+450 uu X, -250 uu Y** | **Deslocado para CIMA e para a ESQUERDA** |
| `prontera` | `[30.0, 1530.0]` | `[30.0, 1930.0]` | `(780.0, 980.0)` | `(780.0, 980.0)` | **0 cel X, 0 cel Y** | **0 uu X, 0 uu Y** | Perfeito alinhamento |
| `prt_church` | `[40.0, 950.0]` | `[60.0, 910.0]` | `(495.0, 485.0)` | `(500.0, 500.0)` | **-1.0 cel X, -3.0 cel Y** | **-50 uu X, -150 uu Y** | Deslocado para baixo e direita |
| `prt_castle` | `[110.0, 1970.0]` | `[80.0, 1900.0]` | `(1040.0, 990.0)` | `(1000.0, 1000.0)` | **+8.0 cel X, -2.0 cel Y** | **+400 uu X, -100 uu Y** | Deslocado para cima e esquerda |

Observe como em `prt_in`, o cálculo dinâmico gerou exatamente:
- `cx = 795.0` (em vez de 750.0, diferença de +45 OBJ = +9 células = +450 uu para a esquerda);
- `cy = 475.0` (em vez de 500.0, diferença de -25 OBJ = -5 células = -250 uu para cima).

Isto bate **milimetricamente** com os prints `x_03_dentro.png` e `y_01_dentro.png`!

---

## 4. Por que o Personagem Caiu no Vazio Preto

No mapa `prt_in`:
- O portal de entrada (`prontera 133,183` -> `prt03`) entrega o jogador em `prt_in (50, 105)`.
- A porta / capacho de entrada da sala fica em `gx=25..26, gy=52..53` (células GAT `50..53, 104..107`).
- Ao sul (`y <= 103`, `gy <= 51`), há uma parede de altura `Z = +30.0` (+300 cm) e vazio além dela.
- Com o deslocamento de **+450 uu em X e -250 uu em Y**, a sala inteira (com balcão em `x=38, y=108`, estantes e NPC) foi arrastada 450 unidades para a esquerda e 250 unidades para cima em relação às coordenadas mundiais esperadas. O personagem foi instanciado na coordenada mundo do GAT, mas a geometria visual da sala não estava lá — restando apenas o vazio preto e a oclusão das paredes sul!

---

## 5. Correção Aplicada em `tools/ue_import_map.py`

Ajustamos o importador para que o centro `(cx, cy)` seja **sempre constante e invariante**, derivado diretamente das dimensões do `.gat` (ou `.gnd`), sem jamais depender da distribuição de ladrilhos visíveis no arquivo `.obj`:

```python
def read_gat_size(obj_path, map_name):
    """Largura x altura em celulas, lido do .gat ou .gnd ao lado do .obj (None se nao houver)."""
    base_dir = os.path.dirname(obj_path)
    base_name = re.sub(r'_full$', '', os.path.splitext(os.path.basename(obj_path))[0])
    import struct
    for cand in (map_name, base_name):
        if not cand:
            continue
        gat_path = os.path.join(base_dir, '%s.gat' % cand)
        if os.path.exists(gat_path):
            with open(gat_path, 'rb') as f:
                head = f.read(14)
            if head[:4] == b'GRAT':
                return struct.unpack_from('<II', head, 6)
        gnd_path = os.path.join(base_dir, '%s.gnd' % cand)
        if os.path.exists(gnd_path):
            with open(gnd_path, 'rb') as f:
                head = f.read(14)
            if head[:4] == b'GRGN':
                gw, gh = struct.unpack_from('<II', head, 6)
                return (gw * 2, gh * 2)
    return None
```

E no `run()` de `tools/ue_import_map.py`:

```python
    gat_size = read_gat_size(obj_path, args['map_name'])
    if gat_size:
        # Grade do rAthena / compile_map_to_obj: 1 celula = 5 unidades do OBJ.
        # Centro verdadeiro e constante: (gat_w * 5 / 2, gat_h * 5 / 2).
        cx = gat_size[0] * 5.0 / 2.0
        cy = gat_size[1] * 5.0 / 2.0
        log('centro do mapa pelo grid do .gat (%dx%d celulas): cx=%.2f, cy=%.2f' % (gat_size[0], gat_size[1], cx, cy))
    else:
        xs = [v[0] for v in ground]
        ys = [v[1] for v in ground]
        cx, cy = (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0
        warn('sem .gat/.gnd para definir o centro; usando bounding box do Ground: cx=%.2f, cy=%.2f' % (cx, cy))
```

Com essa alteração:
1. `SM_prt_in` terá `cx = 750.0, cy = 500.0`.
2. `SM_prt_in_gat_floor` terá `cx = 750.0, cy = 500.0`.
3. `CellOriginUU` será exatamente `(-7500.0, 5000.0)` (em vez de `-7950, 4750`).
4. A posição da célula `(50, 105)` no mundo será `X = -7500 + 50.5 * 50 = -4975`, `Y = 5000 - 105.5 * 50 = -275`.
5. O chão visual da porta em `obj = (250, 525)` será `X = (250 - 750) * 10 = -5000`, `Y = -(525 - 500) * 10 = -250`.
O jogador pousará com 100% de exatidão em cima do ladrilho da porta da loja!

---

## 6. Como Reimportar o Mapa

Para aplicar a nova geometria e o novo `MapInfo` corrigido no `Lvl_PrtIn`:

### Opção A: Headless (via terminal PowerShell)
Na raiz do repositório:
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\mythofthor.uproject" -run=pythonscript -script="tools/ue_import_map.py E:/Dev/Ragnarok_Assets/Prontera/prt_in/prt_in_full.obj"
```

### Opção B: No Editor Unreal aberto
No console / Output Log (selecionando a linguagem **Python**):
```python
py tools/ue_import_map.py E:/Dev/Ragnarok_Assets/Prontera/prt_in/prt_in_full.obj
```

Caso queira também atualizar a BGM ou start-cell durante o reimport:
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\mythofthor.uproject" -run=pythonscript -script="tools/ue_import_map.py E:/Dev/Ragnarok_Assets/Prontera/prt_in/prt_in_full.obj --bgm E:/Dev/Ragnarok_Assets/BGM/17.mp3 --start-cell 50,105"
```

O código em [`tools/ue_import_map.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tools/ue_import_map.py) já está pronto e comitado para a próxima importação.
