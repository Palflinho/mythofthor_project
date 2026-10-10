# Referência 18: Sistema de Chat Completo, Comandos e Emoções

> **Fonte:** roBrowserLegacy (`src/UI/Components/ChatBox/ChatBox.js`, `src/Controls/ProcessCommand.js`, `src/DB/Emotions.js`, `src/UI/Components/Emoticons/Emoticons.js`), Open-Midgard (`src/ui/UIChatWnd.cpp`), rAthena (`src/map/clif.cpp`, `src/map/atcommand.cpp`).  
> **Escopo:** Fase 2 (F2) — Interface de mensagens, canais sociais, comandos de barra e balões de emoções.  
> **Prioridade:** P1 (Comunicação entre jogadores e ergonomia de comandos).

---

## 1. Resumo

A interface de Chat é o núcleo de interação social e controle de conveniência do jogador no Ragnarok Online. Ela processa quatro canais principais de conversação (Público Geral, Cochicho Privado, Grupo e Guilda), cada um com cores padronizadas e regras estritas de alcance. Além disso, interpreta a vasta biblioteca de comandos de barra clássicos (`/`), gerencia o histórico de texto com navegação pelas setas do teclado e instancia balões de emoções animados sobre a cabeça dos personagens.

---

## 2. Dados e Constantes

### 2.1. Dimensões da Caixa de Chat (`ChatBox`)
- **Posição Padrão:** Canto inferior esquerdo da tela $(0, \text{ScreenHeight} - 160)$.
- **Dimensões Padrão:** $360 \times 160$ px (redimensionável em altura até 320 px e largura até 600 px).
- **Abas de Filtragem:**
  1. **Geral (*Public*):** Exibe chat público local, anúncios e mensagens do sistema.
  2. **Combate (*Battle Log*):** Registro de danos desferidos e sofridos, experiência adquirida e itens dropados.
  3. **Grupo (*Party*):** Mensagens do canal de grupo (`%`).
  4. **Guilda (*Guild*):** Mensagens do canal de clã/guilda (`$`).
- **Histórico de Entrada:** Armazena os últimos **20 comandos/mensagens digitados**, navegáveis pelas teclas `Seta para Cima` e `Seta para Baixo`.

### 2.2. Canais de Comunicação e Paleta de Cores Hexadecimal Oficial

| Canal | Prefixo de Envio | Cor do Texto no Chat | Cor do Balão Flutuante | Alcance |
|---|:---:|:---:|:---:|---|
| **Público Local** | *Nenhum* | Preto / Azul Escuro (`#000000` / `#000088`) | Branco com borda preta | Células visíveis na tela ($\approx 15 \times 15$ células) |
| **Cochicho Privado** | `"Nome : Mensagem` ou `/w Nome Msg` | Amarelo / Dourado (`#FFFF00` / `#E6B800`) | *Sem balão flutuante* | Global (qualquer mapa do servidor) |
| **Canal de Grupo** | `%Mensagem` | Ciano / Azul Claro (`#00FFFF` / `#33CCFF`) | Ciano claro flutuante | Global entre todos os membros do grupo |
| **Canal de Guilda** | `$Mensagem` | Verde Claro / Esmeralda (`#00FF00` / `#22DD22`) | Verde claro flutuante | Global entre todos os membros da guilda |
| **Transmissão (Broadcast)** | `@broadcast` ou Mensagem de GM | Amarelo Ouro Vivo (`#FFFF00`) | Faixa dourada no topo da tela | Todos os jogadores online no servidor |
| **Aviso de Sistema** | Automático | Vermelho / Magenta (`#FF3333` / `#FF0077`) | *Sem balão* | Mensagem local para o jogador |

### 2.3. Duração e Geometria dos Balões de Fala e Emotes
- **Duração do Balão de Fala:** **4,5 segundos** (ou até o jogador falar novamente). O balão segue os movimentos do ator.
- **Duração do Balão de Emoção:** **2,5 segundos** (animação em loop de 3 a 5 quadros).
- **Dimensão do Ícone de Emoção:** $32 \times 32$ px (texturas extraídas de `data/texture/effect/` ou `.act/.spr`).

---

## 3. Catálogo de Comandos de Barra Clássicos (`/commands`)

### 3.1. Ergonomia e Controles de Jogo
- `/noctrl` ou `/nc`: Alterna o ataque contínuo automático. Quando ativado, 1 único clique com botão esquerdo mantém o personagem golpeando o monstro até a sua derrota, sem necessidade de segurar a tecla `Ctrl`.
- `/noshift` ou `/ns`: Permite conjurar magias e habilidades de cura/suporte diretamente sobre monstros sem precisar segurar `Shift`.
- `/snap`: Ativa o travamento magnético do cursor do mouse em monstros próximos.
- `/itemsnap`: Ativa o travamento magnético do cursor em itens caídos no solo.
- `/sit` ou `/stand`: Alterna entre a postura sentada e em pé (atalho: tecla `Insert`).
- `/doridori`: Gira a cabeça do personagem rapidamente de um lado para o outro. No cliente oficial, executar `/doridori` consecutivamente sentado **dobra a velocidade de recuperação natural de SP**.
- `/bangbang`: Gira o personagem $45^\circ$ no sentido horário.
- `/bingbing`: Gira o personagem $45^\circ$ no sentido anti-horário.
- `/where`: Imprime as coordenadas cartesianas e o nome do mapa atual no chat (ex: `Prontera City (prontera) : 156, 191`).
- `/who`: Exibe o número total de jogadores conectados ao servidor no momento.

### 3.2. Áudio e Gráficos
- `/sound`: Liga ou desliga todos os efeitos sonoros WAV de golpes, passos e habilidades.
- `/bgm`: Liga ou desliga a música de fundo MP3 do mapa.
- `/effect`: Liga ou desliga todos os efeitos procedurais e de partículas.
- `/mineffect`: Ativa o modo de efeitos gráficos simplificados (reduz poluição visual em GdE/WoE).
- `/miss`: Alterna a exibição da animação de "Miss" em golpes esquivados.
- `/fog`: Liga ou desliga a neblina volumétrica de profundidade da câmera.
- `/lightmap`: Alterna a iluminação dinâmica e sombreamento estático do solo.
- `/aura`: Alterna a Aura do Nível 99 entre Modo 1 (simplificado) e Modo 2 (normal).
- `/aura2`: Liga ou desliga totalmente a exibição da aura de nível 99.

### 3.3. Grupo e Guilda
- `/organize <Nome>`: Cria um novo grupo com o nome fornecido.
- `/invite <Nome>`: Convida um jogador pelo nome para ingressar no seu grupo.
- `/leave`: Abandona o grupo atual.
- `/guild <Nome>`: Cria uma nova guilda (exige 1 *Emperium* no inventário).
- `/breakguild`: Desfaz a guilda atual (apenas para o Líder com todos os membros expulsos).

---

## 4. Tabela de Emoções (Atalhos e Comandos)

| Tecla de Atalho | Comando de Barra | Emoção Visual |
|:---:|---|---|
| **Alt+1** | `/!` ou `/exclamation` | Ponto de exclamação vermelho (!) pulsante |
| **Alt+2** | `/?` ou `/question` | Ponto de interrogação azul (?) com dúvida |
| **Alt+3** | `/love` ou `/heart` | Coração vermelho apaixonado pulsante |
| **Alt+4** | `/sob` ou `/cry` | Cataratas de lágrimas de choro |
| **Alt+5** | `/omg` ou `/shock` | Rosto de espanto com queixo caído |
| **Alt+6** | `/sweat` ou `/heh` | Gota de suor frio na têmpora |
| **Alt+7** | `/kiss` | Beijo de lábios vermelhos flutuando |
| **Alt+8** | `/gg` ou `/money` | Saco de Zeny com cifrão dourado |
| **Alt+9** | `/thx` ou `/thanks` | Vênia de agradecimento com palmas |
| **Alt+0** | `/help` ou `/sos` | Bandeira branca de socorro / pedido de ajuda |

*Comandos adicionais:* `/e1` a `/e34` cobrem toda a grade secundária (dormindo `...zzz`, raiva com veia estourada `/angry`, risada `^^`, lâmpada de ideia `/idea`, dado de sorte `/dice`, etc.).

---

## 5. Entradas e Saídas (Protocolo de Pacotes)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_NOTIFY_CHAT` (`0x008d`):** Fala pública de qualquer jogador nas proximidades `{ GID, message }`.
- **`ZC_WHISPER` (`0x0097`):** Mensagem de cochicho recebida de outro jogador `{ fromName, message }`.
- **`ZC_NOTIFY_CHAT_PARTY` (`0x00ff`):** Mensagem recebida no canal de grupo.
- **`ZC_NOTIFY_EMOTION` (`0x00c0`):** Notifica que uma entidade em cena disparou uma emoção `{ GID, emotionId }`.

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_REQUEST_CHAT` (`0x008c`):** Envia mensagem pública local no chat.
- **`CZ_WHISPER` (`0x0096`):** Envia mensagem privada para um jogador `{ targetName, message }`.
- **`CZ_REQUEST_CHAT_PARTY` (`0x00fe`):** Envia mensagem no canal de grupo.
- **`CZ_REQUEST_EMOTION` (`0x00bf`):** Envia disparo de emoção pelo jogador `{ emotionId }`.

---

## 6. Como Testar

1. **Fala Pública e Balão:** Digitar `"Olá, Prontera!"` no chat. O texto deve surgir em preto na aba Geral e um balão branco com o texto deve permanecer flutuando sobre a cabeça do personagem por 4,5 segundos.
2. **Cochicho:** Digitar `"/w JogadorTeste Oi amigo"`. A mensagem enviada e a resposta recebida devem ser exibidas em amarelo dourado.
3. **Canais com Prefixo:**
   - Digitar `%Para o grupo`: conferir se a mensagem vai em ciano para o canal de grupo.
   - Digitar `$Para a guilda`: conferir se a mensagem vai em verde para o canal de guilda.
4. **Comandos de Ergonomia:**
   - Digitar `/where`: conferir se o chat exibe o mapa e as coordenadas exatas do personagem.
   - Pressionar `Insert` ou digitar `/sit`: verificar se o personagem senta imediatamente e o sprite muda para o quadro sentado.
5. **Emoções:** Pressionar `Alt+1` a `Alt+5`: verificar se os balões animados de exclamação, interrogação, coração e choro surgem com suavidade sobre a cabeça do personagem.
