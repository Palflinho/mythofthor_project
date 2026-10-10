# Especificação 07: Sons e Música de Fundo (BGM & SFX)

> **Origem das referências:**
> - OM: `src/audio/Audio.h`, `src/audio/Audio.cpp`, `src/audio/AudioBackend.h`
> - RB: `src/Audio/SoundManager.js`, `src/Renderer/Entity/EntitySound.js`, `src/Renderer/Map/Sounds.js`, `src/DB/Items/WeaponSoundTable.js`, `src/DB/Items/WeaponHitSoundTable.js`, `src/DB/Jobs/JobHitSoundTable.js`
> - KO: `korangar/client/src/audio/mod.rs`

---

## 1. Resumo

O sistema de áudio do cliente Ragnarok Online opera em duas frentes independentes: reprodução contínua de música ambiente (BGM em MP3 em looping perpétuo, roteada pelo mapa ativo) e execução posicional/2D de efeitos sonoros em formato WAV. Os efeitos de ator são disparados estritamente sincronizados aos quadros do arquivo `.act` ou por eventos de combate do servidor. Os sons de ataque e impacto são resolvidos por tabelas de armamento e classe do receptor. Todos os sons no mundo 3D sofrem atenuação espacial com raio de corte exato de 25 células (250 unidades de mundo).

---

## 2. Dados e Constantes

### 2.1 Volumes e Limites

| Parâmetro | Valor | Unidade | Descrição | Origem |
|---|---|---|---|---|
| `kMaxVolume` (BGM) | `127` | Inteiro (0–127) | Escala nativa Miles Audio do cliente | OM `Audio.cpp:14` |
| `m_soundVolume` (SFX) | `1.0` | Float (0.0–1.0) | Volume mestre global de efeitos | OM `Audio.cpp:46` |
| `volumeMinDist` | `40` | Unidades (4 células) | Raio sem atenuação (volume = 100%) | OM `Audio.cpp:117` |
| `volumeMaxDist` | `250` | Unidades (25 células) | Raio limite audível; além dele, som é descartado | OM `Audio.cpp:113`, RB `SoundManager.js:192` |
| Volume mínimo audível | `0.1` (10%) | Float | Piso de volume para sons dentro do raio máximo no RB | RB `SoundManager.js:192` |
| Fallback BGM | `"bgm\\01.mp3"` | Caminho | Trilha padrão se o mapa não constar na tabela | OM `Audio.cpp:258` |

### 2.2 Tabela de Efeitos de Balanço de Arma (`WeaponSoundTable`)

Disparado no quadro do `.act` que contém o identificador especial `"atk"`:

| Tipo de Arma (`WeaponType`) | Arquivo WAV Disparado | Origem |
|---|---|---|
| Desarmado (`NONE / FIST / KNUCKLE`) | `attack_fist.wav` | RB `WeaponSoundTable.js:18,30` |
| Adaga (`SHORTSWORD`) | `attack_short_sword.wav`, `attack_short_sword_.wav` | RB `WeaponSoundTable.js:19` |
| Espada 1M (`SWORD`) / Shuriken | `attack_sword.wav` | RB `WeaponSoundTable.js:20,40` |
| Espada 2M (`TWOHANDSWORD`) | `attack_twohand_sword.wav` | RB `WeaponSoundTable.js:21` |
| Lança 1M / 2M (`SPEAR`, `TWOHANDSPEAR`) | `attack_spear.wav` | RB `WeaponSoundTable.js:22,23` |
| Machado 1M / 2M (`AXE`, `TWOHANDAXE`) | `attack_axe.wav` | RB `WeaponSoundTable.js:24,25` |
| Maça 1M / 2M (`MACE`, `TWOHANDMACE`), Instrumento | `attack_mace.wav` | RB `WeaponSoundTable.js:26,27,31` |
| Cajado 1M / 2M (`ROD`, `TWOHANDROD`) | `attack_rod.wav` | RB `WeaponSoundTable.js:28,41` |
| Arco (`BOW`) | `attack_bow1.wav`, `attack_bow2.wav` | RB `WeaponSoundTable.js:29` |
| Chicote (`WHIP`) | `attack_whip.wav` | RB `WeaponSoundTable.js:32` |
| Livro (`BOOK`) | `attack_book.wav` | RB `WeaponSoundTable.js:33` |
| Katar (`KATAR`) | `attack_katar.wav` | RB `WeaponSoundTable.js:34` |
| Armas Duplas (ex: Adaga + Espada) | `attack_mace.wav` (ou som de balanço padrão duplo) | RB `WeaponSoundTable.js:43-48` |

### 2.3 Tabela de Impacto no Alvo (`WeaponHitSoundTable` & `JobHitSoundTable`)

Disparado quando a vítima recebe dano (`ZC_NOTIFY_ACT / ZC_NOTIFY_ACT2`):

| Categoria | Arquivo WAV | Condição / Detalhes | Origem |
|---|---|---|---|
| Dano desarmado | `_hit_fist1.wav` a `_hit_fist4.wav` | Sorteio randômico entre 1 e 4 | RB `WeaponHitSoundTable.js:18` |
| Impacto de lâmina (adaga, espada 1M/2M) | `_hit_sword.wav` | Golpe cortante de arma branca | RB `WeaponHitSoundTable.js:19-21` |
| Impacto de lança | `_hit_spear.wav` | Perfuração de lança | RB `WeaponHitSoundTable.js:22-23` |
| Impacto de machado | `_hit_axe.wav` | Golpe pesado cortante | RB `WeaponHitSoundTable.js:24-25` |
| Impacto de maça / chicote / katar / livro | `_hit_mace.wav` | Impacto contundente seco | RB `WeaponHitSoundTable.js:26-34` |
| Impacto de cajado | `_hit_rod.wav` | Impacto de bastão mágico | RB `WeaponHitSoundTable.js:28,41` |
| Impacto de flecha | `_hit_arrow.wav` | Perfuração de projétil | RB `WeaponHitSoundTable.js:29` |
| Reação do jogador (Tecido / Roupas) | `player_clothes.wav` | Aprendiz, Mago, Noviço, Mercador, Bruxo, Sacerdote, Alquimista, Espiritualista | RB `JobHitSoundTable.js:19-35` |
| Reação do jogador (Metal / Armadura) | `player_metal.wav` | Espadachim, Cavaleiro, Templário, Monge, Mestre Taekwon | RB `JobHitSoundTable.js:20,25,29,30,42` |
| Reação do jogador (Couro / Madeira) | `player_wooden_male.wav` / `player_wooden_female.wav` | Arqueiro, Gatuno, Caçador, Mercenário, Arruaceiro, Bardo, Odalisca, Ninja, Taekwon | RB `JobHitSoundTable.js:22,24,28,32,38` |

### 2.4 Efeitos Sonoros de Interface (UI) e Sistema

| Evento | Arquivo de Áudio | Comportamento |
|---|---|---|
| Clique em botão de UI | `effect\btn_click.wav` ou `cursor.wav` | Reprodução 2D imediata |
| Abrir janela básica | `effect\window_open.wav` | Reprodução 2D |
| Fechar janela / Cancelar | `effect\window_close.wav` | Reprodução 2D |
| Equipar item na janela de Equip | `effect\item_equip.wav` | Ao soltar item no slot válido |
| Desequipar item | `effect\item_unequip.wav` | Ao arrastar para inventário ou duplo-clique |
| Coletar item do chão | `effect\item_pickup.wav` | Ao processar sucesso de clique no drop |
| Descartar / Dropar item | `effect\item_drop.wav` | Ao confirmar soltura no chão |
| Alocar ponto de atributo | `effect\status_up.wav` | Ao clicar no botão [+] da janela Alt+A |
| Alocar ponto de habilidade | `effect\skill_up.wav` | Ao clicar no botão [+] da janela Alt+S |
| Subir de Nível (Base / Classe) | `effect\levelup.wav` | Disparado junto com anel de partículas |
| Mensagem de erro / Inválido | `effect\error.wav` | Ação bloqueada pelo cliente |
| Mensagem privada (Whisper) | `effect\whisper.wav` | Recebimento de PM na aba chat |
| Entrada no jogo / Seleção de Personagem | `effect\login.wav` | Ao confirmar seleção de slot |

---

## 3. Regras de Comportamento

### 3.1 Sons Integrados ao Arquivo de Ação (`.act`)
1. **Estrutura interna:** No formato `.act` do Ragnarok, o arquivo possui uma lista de strings de áudio (`act.sounds`).
2. **Gatilho de quadro:** Cada quadro individual (`CAnimationFrame`) armazena um índice inteiro `sound`. Se `sound >= 0`, indica que ao atingir aquele quadro exato durante a animação (`ProcessMotion`), o arquivo correspondente deve ser disparado.
3. **Casos especiais do `.act`:**
   - Para personagens jogadores, o som de ataque é registrado como o identificador dinâmico `"atk"`. O cliente intercepta essa string e a substitui pelo arquivo correspondente da arma atualmente equipada (`WeaponSoundTable`). Se estiver desarmado, toca `attack_fist.wav`.
   - Para monstros, os sons no `.act` são arquivos WAV explícitos (ex: `poring_attack.wav`, `poring_move.wav`, `poring_die.wav`).
4. **Prevenção de repetição múltipla:** O cliente memoriza `_lastActionId`, `_lastAnimationId` e `_lastFileName`. Se a mesma animação continuar ativa sem ter completado seu ciclo (`freeOnAnimationEnd`), o som não é redisparado em cada tick do motor gráfico (`EntitySound.js:46-52`).

### 3.2 Atenuação Espacial 3D

1. **Cálculo da distância euclidiana:**
   $$\text{dist} = \sqrt{(X_{\text{som}} - X_{\text{ouvinte}})^2 + (Y_{\text{som}} - Y_{\text{ouvinte}})^2 + (Z_{\text{som}} - Z_{\text{ouvinte}})^2}$$
2. **Corte rígido (Far Cull):**
   - Se $\text{dist} \ge 250$ unidades (ou $> 25$ células): o áudio é **sumariamente descartado**, retornando imediatamente sem alocar canais de hardware (`Audio.cpp:113`).
3. **Curva de atenuação de volume:**
   - Se $\text{dist} \le 40$ unidades (4 células): $\text{Fator} = 1.0$ (volume pleno).
   - Se $40 < \text{dist} < 250$:
     $$\text{Fator} = \frac{40.0}{\text{dist}}$$
   - Volume final entregue ao mixer: $\text{VolFinal} = \text{Clamp}(\text{VolBase} \times \text{Fator} \times \text{VolumeMestre}, 0.0, 1.0)$.
4. **Pets e Exceções:**
   - Entidades marcadas como mascote/pet não disparam sons de passos para evitar saturação acústica (`EntitySound.js:41-43`).

### 3.3 Gestão de Trilha Sonora de Mapa (BGM)

1. **Leitura da tabela `data/mp3nametable.txt`:**
   - O arquivo possui linhas no formato: `<nome_do_mapa>.rsw#<caminho_do_arquivo_mp3>#`.
   - Exemplo:
     ```text
     prontera.rsw#bgm\03.mp3#
     prt_fild08.rsw#bgm\08.mp3#
     izlude.rsw#bgm\06.mp3#
     ```
2. **Normalização de caminhos:**
   - Nomes de arquivos são convertidos para minúsculas e barras normatizadas (`/` para `\`).
3. **Comportamento na Troca de Mapa:**
   - Ao transicionar de mapa (`ZC_NOTIFY_ACTORINIT` / `ZC_ACCEPT_ENTER`):
     - O cliente resolve o caminho do MP3 para o novo mapa via `ResolveMapBgmPath`.
     - **Regra de Continuidade:** Se o caminho resolvido for **idêntico** ao que já está tocando atualmente (`m_bgmPath == novoPath`), a trilha **continua tocando sem interrupção nem reinício do tempo**.
     - Se o caminho for diferente: a música atual é parada imediatamente, o novo arquivo é carregado da pasta `BGM/` e iniciado em modo de repetição infinita (`loop = true`).
4. **Volume BGM independente:**
   - O volume de BGM possui controle separado do volume de efeitos, variando de 0 a 127 no protocolo clássico (`SetBgmVolume`).

---

## 4. Entradas e Saídas

### 4.1 Entradas de Rede e Disparos
- **`ZC_NOTIFY_ACT / ZC_NOTIFY_ACT2`:** Dispara o som de impacto no receptor (`WeaponHitSoundTable` se for golpe físico de arma, ou `JobHitSoundTable` como camada de armadura) na posição da vítima.
- **`ZC_USE_SKILL`:** Dispara o som associado à habilidade (registrado no efeito de skill ou no `.act`).
- **`ZC_ITEM_PICKUP_ACK`:** Dispara `effect\item_pickup.wav` em 2D quando o jogador coleta um drop.
- **`ZC_PAR_CHANGE` (Nível):** Dispara `effect\levelup.wav`.

### 4.2 Arquivos Lidos no Disco
- `data/mp3nametable.txt` (tabela de mapeamento BGM).
- `BGM/*.mp3` (arquivos de streaming contínuo).
- `data/wav/*.wav` ou `effect/*.wav` (efeitos sonoros one-shot em memória ou cache).

---

## 5. Diferenças entre Referências

1. **Backend de Reprodução:**
   - **OM:** Utiliza *Miles Sound System* (MSS) na arquitetura legada ou *miniaudio* moderno em C++ (`AudioBackend.h`). Volume BGM de 0 a 127.
   - **RB:** Utiliza *Web Audio API* com `AudioContext` nativo do navegador; atenuação com piso mínimo de 0.1 (`Math.max(..., 0.1)`).
   - **KO:** Utiliza a biblioteca Rust *rodio* ou *kira* com instâncias espaciais de som 3D e listener centralizado na câmera/personagem.
2. **Atenuação 3D:**
   - **OM:** Usa a fórmula clássica $40 / \text{dist}$ com corte estrito aos 250 world units.
   - **RB:** Usa interpolação linear no plano 2D de células: `1 - (dist - 1) / 24` com corte em 25 células.
   - **Veredito para o Unreal:** Ambas são equivalentes em escala (25 células = 250 unidades mundo). No Unreal, o `USoundAttenuation` deve ser configurado com:
     - `Attenuation Function`: Inverse / Linear
     - `Inner Radius`: 400.0 cm (4 células no Unreal)
     - `Falloff Distance`: 2500.0 cm (25 células no Unreal)

---

## 6. Como Testar

1. **Ataque e Armas:**
   - Equipar uma adaga e golpear: ouvir `attack_short_sword.wav` no balanço e `_hit_sword.wav` no impacto.
   - Trocar para arco e disparar: ouvir `attack_bow1.wav` e `_hit_arrow.wav`.
   - Desequipar tudo (mão limpa): ouvir `attack_fist.wav` e sorteio de `_hit_fist1.wav` a `_hit_fist4.wav`.
2. **Atenuação Espacial:**
   - Fazer um monstro bater a 3 células do jogador: volume alto e nítido.
   - Afastar o jogador para 20 células: som audível, porém distante.
   - Afastar para 26 células: silêncio absoluto do combate do monstro.
3. **BGM em Portais:**
   - Transicionar de `prontera` para `prt_fild08`: a música troca de `03.mp3` para `08.mp3`.
   - Transicionar entre mapas que compartilham a mesma BGM: a faixa musical não deve sofrer interrupção nem recomeçar do segundo 0.
