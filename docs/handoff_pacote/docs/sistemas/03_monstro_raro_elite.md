# Sistema 03: Monstros Raros e Elites (Sistema Campeão ★)

> **Projeto:** Myth of Thor  
> **Arquivos do rAthena:**  
> - `src/map/mob.cpp` (`mob_spawn_dataset`, `mob_dead`)  
> - `server/npc/custom/myth/elite_mob_system.txt` (controle de drops e anúncios de spawn)  
> - Pacotes no cliente: `ZC_SPAWN_UNIT` (`0x09fe`), `ZC_IDLE_UNIT` (`0x09ff`), `ZC_NOTIFY_EFFECT2` (`0x01f3`)

---

## 1. Comportamento para o Jogador

1. **Aparição no Mapa:**
   - Todo monstro comum tem uma chance de **2.5% (1 em cada 40 spawns)** de nascer como uma variante **Elite / Campeão**.
   - O monstro tem seu nome prefixado com uma estrela dourada:
     - Exemplo: `★ Poring`, `★ Fabre`, `★ Esporo`
2. **Presença Visual Imponente:**
   - **Escala Ampliada:** O sprite é renderizado com **30% a 40% a mais de tamanho** (escala visual 1.35x), destacando-se imediatamente na tela.
   - **Aura Mística:** Efeito visual sutil de partículas ao redor da criatura (`EF_AURABLADE` ou `EF_SNOW`).
3. **Poder e Desafio:**
   - **HP:** 4x maior que o monstro comum.
   - **ATQ Físico / Mágico:** 1.5x maior.
   - **Precisão / Esquiva:** +15 pontos base.
4. **Recompensas Exclusivas:**
   - **Experiência (Base e Job):** 6x maior que o monstro normal.
   - **Taxa de Drops Normais:** 3x a taxa base.
   - **Drop Exclusivo de Síntese:** Chance garantida (100%) de derrubar **1 a 3 Fragmentos de Síntese**, interligando diretamente a caça de elites com o Sistema de Síntese (`01_sintese_random_options.md`).
   - Chance especial de derrubar o equipamento já com opções aleatórias garantidas (*pre-identified*).

---

## 2. Regras e Fórmulas

### 2.1 Limites e Taxas de Spawn

| Parâmetro | Valor | Descrição |
|---|---|---|
| **Chance de Ativação** | `250 / 10000` (2.5%) | Chance calculada no momento de nascimento no mapa |
| **Limite Máximo por Mapa** | 3 Elites simultâneos | Impede acúmulo desmedido de elites em um único mapa |
| **Tempo de Vida** | 30 minutos | Se não for derrotado em 30 min, desaparece para renovar respawn |
| **Anúncio no Mapa** | Mensagem de broadcast local | Mensagem âmbar para o mapa ao nascer elite de monstro Nv $\ge 40$ |

### 2.2 Modificadores de Atributos do Elite

$$\text{HP}_{\text{Elite}} = \text{HP}_{\text{Base}} \times 4$$
$$\text{ATQ}_{\text{Elite}} = \text{ATQ}_{\text{Base}} \times 1.5$$
$$\text{EXP}_{\text{Elite}} = \text{EXP}_{\text{Base}} \times 6$$
$$\text{DropRate}_{\text{Elite}} = \min(10000, \text{DropRate}_{\text{Base}} \times 3)$$

---

## 3. Implementação Limpa no rAthena

Para manter o rAthena facilmente atualizável, a lógica é isolada via diretiva `#ifdef MYTH_ELITE_SYSTEM` em `src/map/mob.cpp`:
1. No `mob_spawn_dataset`:
   - Se o monstro for do tipo normal (não-chefe, não-MVP, não-invocação):
   - Sorteia `rnd() % 10000 < 250`.
   - Se ativo:
     - `md->special_state.size = 2;` (tamanho grande)
     - `snprintf(md->name, sizeof(md->name), "★ %s", md->db->name.c_str());`
     - Aplica multiplicadores nos campos de status.
2. No `mob_dead`:
   - Se `md->special_state.size == 2`:
     - Dispara o drop de Fragmento de Síntese e invoca evento OnEliteKilled para o Códice de Mímir.

---

## 4. Comunicação com o Cliente

1. **Nome no Pacote:**
   - O campo `name` nos pacotes de unidade (`0x09fe`, `0x09ff`, `0x09fd`) traz a string `"★ <Nome>"`. O cliente exibe o nome com a estrela e cor dourada na barra de alvo do HUD.
2. **Escala do Sprite (Unreal Engine 5):**
   - No `MythNetWorld.cpp` / `MythPlayerController.cpp`, quando uma unidade tem prefixo `★` ou flag de tamanho, o ator PaperZD/Flipbook recebe:
     `SetActorScale3D(FVector(1.35f, 1.35f, 1.35f))`
   - Isso escala perfeitamente o sprite 2D no mundo 3D sem quebrar a colisão ou os pontos de ancoragem da sombra!
