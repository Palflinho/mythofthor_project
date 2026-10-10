# Sistema 05: HUD Sem Comandos (Mapeamento de @comandos para Interface Gráfica)

> **Projeto:** Myth of Thor  
> **Arquivos do rAthena:**  
> - `conf/groups.conf` (permissões de comandos para o grupo de jogador comum `id: 0`)  
> - `src/map/atcommand.cpp` (processamento interno)  
> - Pacotes no cliente: `CZ_GLOBAL_MESSAGE` (`0x00f3`), `ZC_NOTIFY_CHAT` (`0x008e`), `ZC_DISP_MESSAGE` (`0x02c1`)

---

## 1. Comportamento para o Jogador

No Ragnarok clássico de servidores privados, os jogadores são obrigados a memorizar e digitar dezenas de comandos com `@` no chat (`@autoloot`, `@alootid`, `@showexp`, `@noask`, `@rates`, `@whereis`...).

No **Myth of Thor**, o jogador **nunca precisa digitar comandos de barra ou arroba no chat para jogar confortavelmente**:
- Todos os comandos úteis são transformados em **botões, alternadores (toggles), menus suspensos e janelas visuais** integradas à HUD do Unreal Engine.
- A digitação no chat permanece exclusivamente para conversas e comandos administrativos de Game Master.

---

## 2. Tabela de Mapeamento: Botão na HUD $\leftrightarrow$ Comando de Servidor

| Elemento Visual na HUD | Ação do Jogador | Comando Enviado ao Servidor via `CZ_GLOBAL_MESSAGE` | Resposta do Servidor |
|---|---|---|---|
| **Toggle "Coleta Automática"** (Menu Opções / HUD) | Ligar / Desligar botão | `@autoloot 100` / `@autoloot 0` | Mensagem do sistema confirmando status |
| **Lista de Itens Desejados** (Filtro Alootid) | Digitar nome do item e clicar [+] | `@alootid +<ItemID>` | Mensagem confirmando inclusão |
| **Toggle "Mostrar EXP"** | Ligar / Desligar | `@showexp` | Exibe ganhos de EXP no chat |
| **Toggle "Modo Silencioso / Sem Pedidos"** | Ativar modo foco | `@noask` | Rejeita automaticamente convites de grupo/trade |
| **Painel de Informações do Servidor** | Clicar em "Info do Reino" | `@rates`, `@uptime`, `@time` | Servidor devolve taxas de EXP e horário do servidor |
| **Localizador de Criaturas** (no Códice de Mímir) | Clicar em "Localizar" | `@whereis <MobID>` | Lista de mapas de spawn e densidade |
| **Localizador de Drops** (no Códice de Mímir) | Clicar no item | `@whodrops <ItemID>` | Lista de monstros que derrubam o item |
| **Pontos de Retorno (Teleporte Salvo)** | Clicar em "Retornar ao Ponto Salvo" | `@load` (se habilitado) ou Asas de Borboleta | Teleporte para a Kafra |

---

## 3. Segurança e Permissões no rAthena

1. **Permissões em `conf/groups.conf`:**
   - O grupo de jogadores padrão (`id: 0`, `"Player"`) possui liberado apenas os comandos seguros que afetam estritamente a própria sessão do jogador:
     ```hcl
     commands: {
         autoloot: true
         alootid: true
         showexp: true
         noask: true
         rates: true
         whereis: true
         whodrops: true
         time: true
         uptime: true
     }
     ```
   - Comandos com risco de abuso (como `@warp`, `@item`, `@kill`, `@speed`) permanecem restritos aos grupos de GM (`id: 99`).
2. **Execução Transparente pelo Cliente:**
   - O cliente Unreal monta o pacote canônico de chat `CZ_GLOBAL_MESSAGE` (`0x00f3`) com o texto correspondente:
     `char_name : @autoloot 100`
   - O rAthena intercepta o comando no `clif_parse_GlobalMessage`, processa a lógica de jogo no servidor e retorna a resposta formatada em `ZC_NOTIFY_CHAT` (`0x008e`) ou `ZC_DISP_MESSAGE` (`0x02c1`).
   - O cliente atualiza o estado visual do botão (ex: toggle de autoloot fica verde/ativo).

---

## 4. Comunicação com o Cliente (Unreal Engine 5)

1. **Exposição em Blueprint / C++:**
   - No `MythRoHUDWidget.cpp` e `MythRoGameMenu.cpp`:
     - Função `ToggleAutoLoot(bool bEnabled)` $\rightarrow$ chama `NetworkSubsystem->SendChatMessage("@autoloot " + (bEnabled ? "100" : "0"))`.
     - Função `ToggleNoAsk(bool bEnabled)` $\rightarrow$ chama `NetworkSubsystem->SendChatMessage("@noask")`.
     - Função `QueryMonsterLocation(int32 MobID)` $\rightarrow$ chama `NetworkSubsystem->SendChatMessage("@whereis " + FString::FromInt(MobID))`.
