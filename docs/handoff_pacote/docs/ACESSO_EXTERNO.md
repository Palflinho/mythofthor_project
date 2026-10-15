# Jogar com um amigo de fora (teste fechado pela Radmin VPN)

A internet da Vero está atrás de **CGNAT** (o IP público é dividido com outros clientes), então abrir portas no roteador não
adianta. O caminho é uma VPN: você e o amigo entram na mesma rede da **Radmin VPN** e o jogo dele conversa com o seu PC
pelo IP da Radmin (26.x.x.x). Nada fica aberto para a internet.

```
PC do amigo (Myth of Thor) --Radmin VPN--> seu PC 26.x.x.x : 6900 / 6121 / 5121 --portproxy--> WSL (Docker rAthena)
```

## 1. Radmin VPN (você e o amigo)
1. Baixem e instalem a Radmin VPN (site oficial: radmin-vpn.com).
2. Você: **Rede > Criar rede** (nome e senha). O amigo: **Rede > Entrar em uma rede existente** com o mesmo nome e senha.
3. Anote o **IP da Radmin do seu PC** (aparece no topo da janela, começa com 26.).

## 2. Servidor anunciando o IP da Radmin (WSL)
No terminal do WSL, na pasta do projeto:

```bash
bash server/acesso_externo/configurar.sh 26.x.x.x
```

O script troca o `char_ip`/`map_ip` (o endereço que o servidor manda ao jogo depois do login), troca a senha padrão entre
os servidores (`s1/p1`) por uma aleatória e reinicia char e map. Para voltar a ser só local: `configurar.sh 127.0.0.1`.
O seu próprio jogo continua funcionando (ele também passa a usar o IP da Radmin, que aponta para o seu PC).

## 3. Windows: levar as portas da Radmin até o WSL (PowerShell como **administrador**)
O Docker roda dentro do WSL; o Windows só repassa para ele o que chega no IP da Radmin. Troque `26.x.x.x` pelo seu IP:

```powershell
$ip = '26.x.x.x'
foreach ($p in 6900, 6121, 5121) { netsh interface portproxy add v4tov4 listenaddress=$ip listenport=$p connectaddress=127.0.0.1 connectport=$p }
New-NetFirewallRule -DisplayName 'Myth of Thor (Radmin)' -Direction Inbound -Protocol TCP -LocalPort 6900,6121,5121 -RemoteAddress 26.0.0.0/8 -Action Allow
```

- Só as três portas do jogo. **Nunca** repasse a 3311 (banco de dados).
- A regra do firewall só aceita quem vem da rede da Radmin (26.0.0.0/8).
- Para desfazer:
  ```powershell
  foreach ($p in 6900, 6121, 5121) { netsh interface portproxy delete v4tov4 listenaddress=$ip listenport=$p }
  Remove-NetFirewallRule -DisplayName 'Myth of Thor (Radmin)'
  ```
- O Docker para quando o WSL fica ocioso: deixe um terminal do WSL aberto enquanto vocês jogam (ver a nota do "keepalive").

## 4. Pacote do jogo para o amigo
Feche o Unreal e rode (no PowerShell normal, na pasta do projeto):

```powershell
powershell -ExecutionPolicy Bypass -File tools/make_friend_build.ps1 -ServerIp 26.x.x.x
```

Sai em `E:\Dev\MythAmigo\build\Windows`: o jogo empacotado (não precisa da Unreal), as tabelas, os gráficos do RO
(`RagnarokAssets`) e o `Jogar.bat` já apontando para o seu IP. Compacte essa pasta e mande (Google Drive etc.).
O amigo extrai, entra na rede da Radmin e abre `Jogar.bat`. Sua conta de teste **não** vai no pacote; o jogo abre na tela
de login.

## 5. Conta do amigo
Escolha o usuário e a senha com ele e crie no WSL (troque os valores; sexo M ou F):

```bash
docker exec mythofthor-db mariadb -uragnarok -pragnarok ragnarok -e "INSERT INTO login (userid, user_pass, sex, email, group_id) VALUES ('usuario_dele', 'senha_dele', 'M', 'a@a.com', 0);"
```

`group_id 0` = jogador comum (sem comandos de GM).

## Se não conectar
- Na Radmin, os dois aparecem "online" na mesma rede? Teste com `ping 26.x.x.x` do PC dele.
- No seu PC: `netsh interface portproxy show v4tov4` deve listar as três portas.
- `docker ps` no WSL: os quatro containers (db, login, char, map) estão "Up"?
- Logou mas travou em "conectando ao mapa": o `configurar.sh` foi rodado com o IP da Radmin (e não 127.0.0.1)?

## 6. Comandos de GM para testar (conta com group_id 99)
- `@mvps` lista os MVPs (nomes do mob_db), `@mvp <nome>` invoca um MVP perto de você (ex.: `@mvp baphomet`), `@mvpkill` remove os monstros do mapa.
- Os demais comandos normais do rAthena também funcionam: `@item <id> <qtd>`, `@monster`, `@blvl`, `@jlvl`, `@warp`, `@alive`...
- Para dar GM a uma conta: `UPDATE login SET group_id=99 WHERE userid='nome';` (vale no próximo login). GM pode usar comandos que afetam o servidor todo: só para quem você confia.
