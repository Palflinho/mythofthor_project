# Especificação da Arquitetura de Rede: Protocolo rAthena

Este documento descreve as estruturas binárias dos pacotes de rede utilizados na comunicação entre o cliente Unreal Engine 5 e o servidor rAthena.

---

## 🔒 Parâmetros Globais
* **Versão do Pacote (`PACKETVER`):** `20211103`
* **Endereço Padrão (Localhost):** `127.0.0.1`
* **Ordem de Bytes (Endianness):** Little-Endian (`<`)

---

## 1. Conexão com o Login-Server (`Porta 6900`)

### `CA_LOGIN` (0x0064) - Cliente $\rightarrow$ Login-Server
Enviado quando o jogador clica no botão "Entrar" na tela de login.

| Campo | Tipo | Tamanho | Descrição |
| :--- | :--- | :--- | :--- |
| `PacketID` | `uint16` | 2 bytes | `0x0064` |
| `Version` | `uint32` | 4 bytes | `20211103` |
| `Username` | `char[24]` | 24 bytes | Nome de usuário (com preenchimento de zeros) |
| `Password` | `char[24]` | 24 bytes | Senha do usuário |
| `ClientType`| `uint8` | 1 byte | `0` (Cliente padrão) |
| **Total** | | **55 bytes** | |

### `AC_ACCEPT_LOGIN` (0x0069) - Login-Server $\rightarrow$ Cliente
Resposta do servidor autorizando a conexão e enviando a lista de servidores de personagens disponíveis.

---

## 2. Conexão com o Char-Server (`Porta 6121`)

### `CH_ENTER` (0x0065) - Cliente $\rightarrow$ Char-Server
Envia o ID da conta e o token de sessão autenticado pelo Login-Server.

### `HC_CHAR_LIST` - Char-Server $\rightarrow$ Cliente
Retorna os personagens existentes (nome, classe, nível base/classe, atributos).

### `CH_SELECT_CHAR` - Cliente $\rightarrow$ Char-Server
Seleciona o slot do personagem para entrar no mundo.

---

## 3. Conexão com o Map-Server (`Porta 5121`)

### `CZ_ENTER` (0x0072) - Cliente $\rightarrow$ Map-Server
Inicia a sessão de mapa no servidor de zona correspondente (`prontera` ou `new_1-1`).

### `CZ_REQUEST_MOVE` (0x035F) - Cliente $\rightarrow$ Map-Server
Envia a coordenada de destino do clique do mouse no formato compactado de 3 bytes (10 bits para X e 10 bits para Y):
```text
PackedCoords = ((X & 0x3FF) << 14) | ((Y & 0x3FF) << 4)
```

### `ZC_NOTIFY_PLAYERMOVE` - Map-Server $\rightarrow$ Cliente
Confirmação oficial do servidor para o personagem iniciar a rota de caminhada.
