status: pendente

# Resposta 004: Portais para dentro das Casas de Prontera (Status no Servidor e na Unreal)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 003 (Parte 2)

---

## 1. Visão Geral da Arquitetura dos Interiores de Prontera

No Ragnarok Online clássico e no rAthena, **não existem mapas individuais por casa** (não existem arquivos como `casa1.gat` ou `casa2.gat`). Todas as 15 residências urbanas e lojas de Prontera residem em um único mapa interno compartilhado: **`prt_in.gat`**.

Verificamos o servidor rAthena e a árvore de conteúdo da Unreal Engine (`Content/Maps/`):
- **100% dos warps das casas estão ATIVOS** no servidor (`/rathena/npc/warps/cities/prontera.txt`).
- **100% dos mapas internos das casas já estão IMPORTADOS** na Unreal Engine (`Content/Maps/PrtIn/Lvl_PrtIn.umap`).

---

## 2. Matriz Completa Casa por Casa de Prontera

Abaixo está o status detalhado por imóvel, relacionando os warps do servidor (`prontera.txt`) com o nível Unreal:

| Casa / Estabelecimento | Célula em Prontera | Warp Ida (Servidor) | Destino em `prt_in` | Warp Retorno | Mapa Unreal | Status Final |
|---|---|---|---|---|---|---|
| **01. Loja de Ferramentas / Utilidades** | (107, 215) [2x2] | `prt01` (Ativo) | (240, 139) | `prt01-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **02. Residência / Biblioteca Noroeste** | (120, 267) [2x2] | `prt02` (Ativo) | (180, 97) | `prt02-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **03. Loja de Armas (Weapon Shop)** | (133, 183) [2x2] | `prt03` (Ativo) | (50, 105) | `prt03-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **04. Taberna / Pub de Prontera** | (134, 221) [1x1] | `prt04` (Ativo) | (131, 71) | `prt04-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **05. Loja de Armaduras (Armor Shop)** | (177, 221) [2x2] | `prt05` (Ativo) | (168, 128) | `prt05-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **06. Residência Sudeste Central** | (179, 184) [2x2] | `prt06` (Ativo) | (60, 73) | `prt06-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **07. Residência Nordeste** | (192, 267) [2x2] | `prt07` (Ativo) | (178, 55) | `prt07-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **08. Florista / Loja de Presentes** | (204, 192) [2x2] | `prt08` (Ativo) | (68, 134) | `prt08-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **09. Residência Sudeste** | (208, 154) [2x2] | `prt09` (Ativo) | (172, 29) | `prt09-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **10. Residência Extremo Sudoeste** | (42, 67) [2x2] | `prt10` (Ativo) | (44, 29) | `prt10-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **11. Ordem dos Cavaleiros (Knight Guild)**| (45, 346) [2x2] | `prt11` (Ativo) | (80, 110) | `prt11-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **12. Residência Sudoeste A** | (73, 100) [1x1] | `prt12` (Ativo) | (208, 176) | `prt12-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **13. Residência Sudoeste B** | (74, 90) [2x2] | `prt13` (Ativo) | (248, 170) | `prt13-1` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **14. Residência Sudoeste C** | (84, 89) [1x1] | `prt14-1` (Ativo) | (282, 176) | `prt14` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |
| **15. Residência Leste (Próx. Catedral)**| (263, 279) [2x2] | `prt15-2` (Ativo)| (227, 18) | `prt20-2` (Ativo) | `Content/Maps/PrtIn/Lvl_PrtIn.umap` | ✅ **Pronto** |

---

## 3. Grandes Edifícios e Saídas de Campo

Além das 15 casas urbanas:

1. **Catedral de Prontera (`prt_church`):**
   - Entrada: `prt004` em `prontera (237, 317)` -> `prt_church (100, 60)`.
   - Retorno: `prtch001` em `prt_church (100, 56)` -> `prontera (234, 314)`.
   - Passagens internas: `prtch01` e `prtch02` ligando à Sala dos Noviços e Sala dos Padres.
   - Status: **Importado** em `Content/Maps/PrtChurch/Lvl_PrtChurch.umap`.

2. **Castelo Real de Prontera (`prt_castle`):**
   - Entrada: `prt005` em `prontera (156, 360)` -> `prt_castle (102, 20)`.
   - Retorno: `prtca001` em `prt_castle (102, 16)` -> `prontera (156, 356)`.
   - 12 passagens internas entre salões reais ativas no servidor (`prtca01` a `prtca12`).
   - Status: **Importado** em `Content/Maps/PrtCastle/Lvl_PrtCastle.umap`.
   - *Nota:* O portal do fundo do castelo (`prtca002` em `102, 181`) leva a `prt_gld` (Reino das Valquírias / Castelos WoE). O mapa `prt_gld.umap` **não está importado** no momento.

3. **Campos ao Redor da Cidade:**
   - Sul (`prt_fild08`): Portal `prt001` em `(156, 22)`. Importado em `Content/Maps/PrtFild08/Lvl_PrtFild08.umap`.
   - Oeste (`prt_fild05`): Portal `prt002` em `(22, 203)`. Importado em `Content/Maps/PrtFild05/Lvl_PrtFild05.umap`.
   - Leste (`prt_fild06`): Portal `prt003` em `(289, 203)`. Importado em `Content/Maps/PrtFild06/Lvl_PrtFild06.umap`.

---

## 4. O Que Falta para o Jogador Entrar e Sair das Casas na Prática

- **Servidor:** 100% configurado, sem necessidade de alterações.
- **Geometria / Mapas:** 100% dos mapas das casas já existem e estão importados na pasta `Content/Maps/`.
- **Implementação Pendente no Cliente (Unreal):**
  1. **Recepção do Pacote de Troca de Mapa:**
     Quando o jogador pisa no portal no servidor, o rAthena envia o pacote `ZC_NPCACK_MAPMOVE` (`0x0091`) ou `ZC_ACCEPT_ENTER2 / ZC_NOTIFY_MAPINFO` contendo o nome do mapa (`prt_in.gat`) e as coordenadas destino `(x, y)`.
  2. **Transição de Nível:**
     O cliente Unreal precisa carregar `Lvl_PrtIn` (seja via `UGameplayStatics::OpenLevel` ou Level Streaming), spawnar o ator do jogador na célula correspondente convertida para coordenadas de mundo (`(x * 100cm, y * 100cm)`), e enviar o `CZ_NOTIFY_ACTORINIT (0x007d)` de volta para o Map-Server para liberar o início de ações no novo mapa.
