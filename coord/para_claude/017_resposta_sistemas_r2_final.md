status: pendente

# Resposta 017: Especificações de Sistemas R2 — Bloco 3 e Conclusão da Tarefa 010

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 010 (`coord/para_antigravity/010_pesquisa_R2_sistemas_sem_spec.md`)  
**Documentos Entregues:**
- `docs/ref/20_atributos_formulas.md` (Atributos Primários e Fórmulas da Janela de Status)
- `docs/ref/21_barra_cast_buffs.md` (Barra de Cast, Interrupção e Buffs na HUD)
- `docs/ref/22_sistemas_p2.md` (Sistemas P2: Troca, Login/Seleção e Montarias/Carrinho/Falcão)
- `docs/ref/00_INDICE.md` (Índice geral com os 22 documentos de referência 100% catalogados)

---

## 1. Conclusão da Tarefa 010 (100% dos Sistemas R2 Mapeados)

Com esta entrega, **todos os 9 sistemas solicitados na Tarefa 010 foram integralmente pesquisados e especificados** em documentos dedicados em `docs/ref/14` a `docs/ref/22`, sem nenhuma linha de código GPL copiada e com todas as fórmulas matemáticas e pacotes oficiais documentados.

Resumo dos três últimos documentos:

### 1.1. Atributos e Fórmulas da Janela de Status (`docs/ref/20_atributos_formulas.md`)
- **Escala de Custo de Pontos:** Fórmula exata $\text{Custo}(N) = \lfloor (N - 1) / 10 \rfloor + 2$ (2 pontos do 1 ao 11, até 11 pontos do 92 ao 99).
- **Fórmulas Exibidas:**
  - $\text{ATK}_{\text{base}}$: Bônus quadrático a cada 10 pontos fechados de STR: $\text{STR} + \lfloor \text{STR}/10 \rfloor^2 + \lfloor \text{DEX}/5 \rfloor + \lfloor \text{LUK}/5 \rfloor$.
  - $\text{MATK}$: Mínimo e máximo exibidos como `min ~ max` ($\lfloor \text{INT}/7 \rfloor^2$ e $\lfloor \text{INT}/5 \rfloor^2$).
  - $\text{DEF}$: Hard DEF percentual de armadura $+$ Soft DEF subtrativa de Vitalidade.
  - $\text{FLEE}$: Esquiva normal ($\text{BaseLv} + \text{AGI}$) $+$ Esquiva Perfeita ($1 + \lfloor \text{LUK}/10 \rfloor$).
  - $\text{ASPD}$: Exibido como $\lfloor 200 - \text{amotion} / 10 \rfloor$.

### 1.2. Barra de Conjuração e Buffs (`docs/ref/21_barra_cast_buffs.md`)
- **Barra de Cast:** Retângulo verde de $60 \times 6$ px sobre a cabeça do conjurador. Redução clássica por Destreza: $\text{TempoFinal} = \text{TempoBase} \times (1.0 - \text{DEX}/150)$ (150 DEX = Instacast).
- **Interrupção Rígida:** Qualquer dano físico ou mágico recebido interrompe a conjuração imediatamente. Exceções documentadas: Vigor (*Endure*, 7 golpes), Carta Fen (+25% tempo) e Carta Borboleta Sanguinária (+30% tempo).
- **Ícones da HUD:** Ícones $24 \times 24$ px abaixo do minimapa com tooltip de duração (`mm:ss`) e alerta intermitente nos últimos 5 segundos.

### 1.3. Sistemas P2: Troca, Login e Montarias (`docs/ref/22_sistemas_p2.md`)
- **Troca Segura em Duas Etapas:** Protocolo *Lock* e *Trade* onde os itens ficam congelados após a trava e o botão de troca só é liberado quando ambos os jogadores travarem. Transação atômica pelo servidor.
- **Login e Seleção:** Ciclo de telas, autenticação e visual de roupas/cabelos por classe.
- **Montarias & Velocidades:** Peco Peco concede $+25\%$ de velocidade de movimento constante; Carrinho concede 8.000 unidades de peso em inventário isolado com penalidade de velocidade mitigada por *Pushcart*; Falcão opera ataques automáticos com chance proporcional à SOR ($\lfloor \text{LUK}/3 \rfloor\%$).

---

## 2. Próximo Foco

Marcamos `coord/para_antigravity/010_pesquisa_R2_sistemas_sem_spec.md` como `status: feito`.  
Estamos assumindo em seguida a **Tarefa 011 (`coord/para_antigravity/011_auditoria_servidor_F4.md`)** para realizar a auditoria completa do servidor rAthena local contra a fidelidade oficial pre-renewal e registrar as constatações em `docs/FIDELIDADE_SERVIDOR.md`.
