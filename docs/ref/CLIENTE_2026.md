# Cliente kRO 2026-01-07 (pacote do fórum, em `D:\Ragnarok_Assets`)

Registrado em 2026-10-12. Só leitura estática (nada foi executado). Origem: tópicos do fórum do rAthena indicados pelo usuário (149414 e 149611); o conteúdo dos tópicos não foi lido (403).

| Item | Valor |
|---|---|
| Executável | `2026-01-07_Ragexe_1767686776_VHL_clientinfo_fixed.exe`, 16.499.200 bytes |
| SHA-256 | `0A4FBF90F6711CEA8F7DF1BDF69C30DD85516E5BC14E5A7478443A1B3D6D8D4D` |
| Compilação | carimbo do PE 2026-01-06 06:52 UTC; texto "Jan  6 2026"; o número 1767686776 do nome é um horário Unix (2026-01-06) |
| PACKETVER | **não escrito no exe**; por convenção do kRO o Ragexe de 2026-01-07 corresponde a `20260107`. Não há `clientinfo.xml` na pasta (o exe o lê do disco) |
| Modificado por terceiros | sim: sessão WARP com 54 patches (`rAthena_Session_SSOMode.yml`): SSO, DNS, proxy, `CustomDLL`, `GRFsFromIni`, tradução EN, sem anúncios da Gravity, zoom/câmera maiores |
| Dados | `Data2026/data.grf` (4,59 GB, 2026-03-20), `2026.grf` (48 MB), `System/*.lub` até 2026-05-18, `DATA.INI` com `2026.grf` e `data.grf` |
| Classes novas | o exe contém nomes de efeitos e jobs do **Druida** (`effect\DRUID\DR_CRUEL_BITE`, `AT_QUILL_SPEAR`, `DRUID_RIDING`) e de classes de 3ª geração |
| Servidor | o rAthena oficial (commit `d4b8e7b`, 2026-10-06) é o mais novo existente e **não tem o Druida jogável**; `PACKETVER` padrão do código é 20211103 (usamos 20250402) |

Não executar o exe fora de uma VM (DLL personalizada e proxy). Uso possível no futuro: referência de dados (sprites, efeitos, textos) se um dia criarmos classes novas por conta própria; fora do escopo atual (até a transclasse).

## O que o post do fórum diz (colado pelo usuário em 2026-10-12; tópico 149414, autor VictorHug0)

- Release do Ragexe de 2026-01-07 **desempacotado**, com o carregamento do `clientinfo` restaurado, mais uma versão do WARP compatível. Diz que **o cliente suporta nativamente as classes novas**.
- O tamanho do exe informado no post (15,73 MB) bate com o do nosso arquivo (16.499.200 bytes = 15,73 MiB), na versão "DX7 Fix" (editada em 12 de agosto).
- Perfis do WARP: `rAthena_Session.yml` (normal) e `rAthena_Session_SSOMode.yml` (SSO). Nós temos o do SSO.
- Os dois hashes de VirusTotal citados no post (`bbbf2f61...` e `7eedee17...`) **não são** o SHA-256 do nosso arquivo (`0A4FBF90...`). Provavelmente são das versões anteriores à correção DX7. O "falso positivo" é a palavra do autor, não verificado.
- **"Database Package"** (link do Google Drive no post): provavelmente os dados do **servidor** (rAthena) para as classes novas. É a peça que diria se existe Druida jogável no servidor. **Ainda não baixado nem lido.**
- Pedem para não abrir issue sobre configuração de GRF/data; só bugs do exe e dos diffs.

## Leitura do `Data2026` (`D:\Ragnarok_Assets\Data2026_\Data2026`, 2026-10-12) — só listagem dos índices dos GRF

- `2026.grf` (1.170 arquivos) traz `data\clientinfo.xml` e `data\sclientinfo.xml`: **configuração genérica** (serviço `korea`, `version` 55, portas 6101/6900, `langtype` 12 e 0), **sem PACKETVER**. A data do pacote só vem do exe (convenção 20260107).
- `data.grf` (241.336 arquivos) não é o "Database Package" do servidor: é o **dado do cliente**. Os links do post apontam para este mesmo conjunto.
- **Druida no cliente:** corpos `druid_남/여` e `druid_riding_남/여` (`sprite\인간족\몸통`), paletas, escudos, 2.335 texturas de efeito (`texture\effect`), 76 sons de efeito (`wav\effect`), `imf\druid_riding_남.imf` e ícones de interface. As entradas `c_evil_druid_cross` (robes) e `evil_druid_*.wav` são itens/monstro, não a classe.
- Tabelas de classe/skill já compiladas: `jobname.lub`, `pcjobname.lub`, `jobinheritlist.lub`, `skillinfolist.lub`, `idnum2itemdisplaynametable.txt` (precisam de decompilação Lua para ler).
- Conclusão: **o cliente 2026 tem os assets do Druida; o servidor (rAthena oficial) não tem a classe**. Para jogar com ela seria preciso implementar job, skills e balanceamento no servidor.
