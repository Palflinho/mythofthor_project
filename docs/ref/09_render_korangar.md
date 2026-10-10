# Especificação 09: Arquitetura de Renderização e Iluminação (Korangar $\rightarrow$ Unreal HD-2D)

> **Origem das referências (Korangar - Licença MIT):**
> - KO: `korangar/shaders/passes/forward/entity.slang` (Shaders de entidades e sprites)
> - KO: `korangar/shaders/passes/forward/model.slang` (Shaders de modelos 3D e terreno)
> - KO: `korangar/shaders/passes/directional_shadow/` e `point_shadow/` (Geração de mapas de sombra)
> - KO: `korangar/shaders/passes/light_culling/` (Tiled/Clustered lighting)
> - KO: `korangar/src/graphics/passes/forward.rs` e `korangar/src/world/entity/mod.rs`

---

## 1. Resumo

O `Korangar` é um cliente moderno de Ragnarok escrito em Rust sobre WGPU/Vulkan que renderiza o mundo 3D clássico (.rsw/.gnd/.rsm) com iluminação dinâmica avançada e sprites 2D (.spr/.act) integrados ao espaço tridimensional. O Korangar resolve os principais gargalos visuais do RO clássico: gera normais curvadas artificiais para que sprites planos tenham relevo volumétrico perante tochas e luz solar; separa a renderização em pass opaco recortado (com Z-write) e pass translúcido (via WBOIT); projeta e recebe sombras dinâmicas de múltiplos cascades. Esta especificação analisa essas soluções e define a transposição direta para o Unreal Engine 5 no modelo HD-2D do Myth of Thor.

---

## 2. Dados e Constantes do Pipeline Gráfico

| Parâmetro / Recurso | Valor no Korangar | Equivalente no Unreal Engine 5 | Descrição Técnica | Origem |
|---|---|---|---|---|
| **Pipeline de Shading** | Tiled Forward Lighting (`TILE_SIZE = 16`) | Deferred / Clustered Forward Lumen | Agrupamento de luzes pontuais em grades de tela | KO `entity.slang:137` |
| **Normal de Sprite** | Curvatura cilíndrica via `rotateY(view.Z, x)` | Material Expression / Normal Map esférico | Transforma quad plano em semicilindro iluminável | KO `entity.slang:64` |
| **Separação de Passes** | Pass 0 (Opaque Cutout) / Pass 1 (WBOIT) | Blend Mode: **Masked** (com Clip ~0.33) | Pixels de sprite escrevem profundidade no Z-Buffer | KO `entity.slang:128-135` |
| **Epsilon de Opacidade** | `ENTITY_OPAQUE_EPSILON = 0.05` | `OpacityMaskClipValue = 0.33` | Limiar para descartar pixel transparente (`discard`) | KO `entity.slang:128` |
| **Offset de Camadas** | `extra_depth_offset` proporcional por parte | World Position Offset (WPO) ou Camadas Paper2D | Afasta cabelo, elmo e arma para eliminar Z-fighting | KO `entity.slang:74` |
| **Sombras Direcionais** | SDSM (Sample Distribution Shadow Maps) | Virtual Shadow Maps (VSM) / Cascaded Shadow Maps | Silhueta do sprite recortada projeta sombra no chão | KO `entity.slang:165` |
| **Sombras Pontuais** | Depth Cube Array para tochas/lanternas | Point Light Shadow Maps | Iluminação de tochas em masmorras e postes de cidade | KO `entity.slang:25` |
| **Filtragem de Sprite** | Band-limited Pixel Sampling | Texture Group `2D Pixels (Unfiltered)` / Nearest | Evita cintilação de pixel art em movimento de câmera | KO `entity.slang:113` |
| **Vento em Modelos** | `sin(world.x + timer) * wind_affinity` | Material World Position Offset com Senóide | Movimento sutil de copas de árvores e folhagens | KO `model.slang:58` |

---

## 3. Arquitetura e Soluções Técnicas do Korangar

### 3.1 Geração de Normais Artificiais em Sprites 2D
- **O Problema Clássico:** Um sprite 2D puro colocado no mundo 3D é um retângulo plano (quad). Se sua normal for simplesmente perpendicular ao plano, o personagem reflete luz de forma homogênea e artificial, parecendo um pedaço de papelão flutuando no cenário.
- **A Solução do Korangar (`entity.slang:64-65`):**
  - O shader calcula uma rotação da normal ao longo do eixo horizontal do quad (`vertex.position.x`):
    ```slang
    let rotated = rotateY(float3(global_uniforms.view.getColumn(2).x, 0, global_uniforms.view.getColumn(2).z), vertex.position.x);
    output.normal = float3(-rotated.x, rotated.y, rotated.z);
    ```
  - **Efeito Visual:** As bordas laterais do sprite têm normais apontando para os lados e o centro aponta para a câmera, simulando a superfície curva de um cilindro 3D.
  - **Resultado na Luz:** Quando o sol se põe à direita, o lado direito do sprite fica dourado e iluminado, enquanto o lado esquerdo entra em penumbra suave, criando a estética autêntica de jogos HD-2D modernos (como Octopath Traveler e Live A Live).

### 3.2 Otimização de Z-Buffer e Eliminação de Z-Fighting
- **Subcamadas do Ator:** Um personagem de RO é composto por até 5 camadas sobrepostas no mesmo ponto do espaço: corpo, cabeça, acessório de cabeça inferior, médio, superior, arma e escudo.
- **Solução no Korangar (`entity.slang:74-76`):**
  - O shader injeta um deslocamento de profundidade microscópico (`depth_offset` e `extra_depth_offset`) ao longo do vetor de visão da câmera:
    $$\text{ViewPos}_{\text{ajustado}} = \text{ViewPos} - (0, 0, \text{depth\_offset} + \text{curvature\_offset}, 0)$$
  - Cada camada recebe um offset predeterminado na ordem de montagem:
    - Corpo: offset 0
    - Cabeça: offset +1
    - Acessórios: offset +2
    - Arma/Escudo: offset +3
  - Isso garante que todas as camadas escrevam no Z-Buffer com profundidades distintas, permitindo oclusão perfeita com paredes e postes 3D sem qualquer tremulação de Z-fighting.

### 3.3 Sombras Dinâmicas Recebidas e Projetadas
- No cliente clássico de 2008, a sombra é apenas uma textura oval preta semitransparente desenhada no chão (`shadow.bmp`).
- No Korangar:
  - O quad do sprite participa do pass de sombra direcional (`directional_shadow`): a textura recortada com o canal alfa projeta a silhueta exata do personagem no terreno inclinado e em degraus.
  - O fragment shader do sprite também calcula o teste de sombra (`shadow_coords` vs `shadow_maps`): quando o personagem entra debaixo de um telhado ou atrás de um muro em Prontera, o sprite 2D escurece naturalmente porque está sob a sombra da geometria 3D.

---

## 4. O que VALE para o Myth of Thor no Unreal Engine 5

### 4.1 Material Master do Sprite HD-2D (Recomendação para o Claude)
1. **Blend Mode:** Utilizar **Masked** (não Translucent).
   - *Por quê:* Materiais translúcidos no Unreal não escrevem no Z-Buffer padrão, não projetam sombras nítidas via Virtual Shadow Maps (VSM) e sofrem com erros de ordenação de profundidade com múltiplos personagens na tela. Materiais Masked participam de todo o pipeline Lumen e de sombras.
2. **Opacity Mask Clip Value:** Ajustar para `0.33` ou `0.5`.
3. **Cálculo de Normal Artificial no Material Graph:**
   - Criar uma função de material que converte as coordenadas UV horizontais (0 a 1) em um vetor de normal hemisférico:
     $$\text{NormalX} = (\text{TexCoord.U} - 0.5) \times 2.0$$
     $$\text{NormalZ} = \sqrt{\max(0.0, 1.0 - \text{NormalX}^2)}$$
     $$\text{PixelNormal} = \text{TransformLocalVectorToWorld}(\text{Vector3}(\text{NormalX}, 0.0, \text{NormalZ}))$$
   - Isso replica a curvatura do Korangar de forma nativa e extremamente barata dentro do material do Unreal.
4. **Configuração de Textura de Pixel Art:**
   - Mip Gen Settings: `NoMipmaps` (ou Mipmaps com Kernel Sharpening).
   - Texture Group: `2D Pixels (Unfiltered)`.
   - Filter: `Nearest`.
5. **Sombras Híbridas:**
   - Habilitar `Cast Shadow` no componente do sprite para projeção de silhueta real no chão e nas paredes.
   - Manter um decalque sutil ou disco sombreado clássico na base dos pés para ancoragem visual quando o sol estiver a pino ou em ambientes fechados difusos.

---

## 5. O que NÃO VALE (ou Evitar) no Unreal Engine 5

| Abordagem do Korangar | Por que NÃO usar no Unreal | Alternativa Recomendada no Unreal |
|---|---|---|
| **Pipeline Forward Tiled Shading manual em código** | O Unreal Engine 5 possui motor de iluminação de última geração (Lumen / Clustered Forward) otimizado para hardware moderno. | Utilizar os componentes nativos de luz (`DirectionalLight`, `PointLight`, `SpotLight`, `SkyLight`). |
| **Pass WBOIT manual em Slang** | Implementar passes de WBOIT no Unreal exigiria modificar o renderizador nativo em C++ do engine, quebrando compatibilidade com atualizações. | Usar Blend Mode **Masked** para sprites de personagens e Dithered Temporal AA para transparências suaves. |
| **Importação de formatos crus (.rsw, .gnd, .gat) em tempo de execução** | Carregar polígonos brutos em tempo de execução impede uso de Nanite, static batching e pré-computação de colisão física. | Manter a compilação offline de mapas via ferramentas Python (`compile_map_to_obj.py`) para `UStaticMesh` no Content Browser. |
| **Sombras manuais com SDSM e matrizes em software** | O Unreal já gerencia Virtual Shadow Maps (VSM) e Cascaded Shadow Maps com filtros PCSS e suavização temporal automática. | Habilitar `Cast Dynamic Shadow` nos atores e deixar o VSM do UE5 calcular as projeções. |

---

## 6. Como Testar e Validar o Visual HD-2D

1. **Iluminação Volumétrica em Sprites:**
   - Posicionar o personagem em Prontera ao lado de um poste de luz noturno: o lado voltado para o poste deve ficar visivelmente iluminado pela cor quente da lâmpada, enquanto o lado oposto fica sombreado.
2. **Projeção e Recepção de Sombra:**
   - Posicionar o personagem sob o beiral de uma casa: o sprite do personagem deve escurecer ao entrar na área sombreada pela casa.
   - Caminhar sob a luz solar: a silhueta em pixel art do personagem deve ser projetada nitidamente no piso de paralelepípedo da praça.
3. **Ausência de Z-Fighting e Ordenação de Camadas:**
   - Equipar elmo volumoso, cabelo longo e asas: ao girar o personagem em 8 direções e andar, nenhuma camada deve piscar ou atravessar a outra de forma intermitente.
