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
