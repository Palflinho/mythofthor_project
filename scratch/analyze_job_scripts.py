import os
import re

RATHENA = r"\\wsl.localhost\Ubuntu\home\palf\mythofthor-server\rathena"

job_files = [
    # 1-1
    ("1-1", "Swordman", r"npc\pre-re\jobs\1-1\swordman.txt"),
    ("1-1", "Mage", r"npc\pre-re\jobs\1-1\mage.txt"),
    ("1-1", "Archer", r"npc\pre-re\jobs\1-1\archer.txt"),
    ("1-1", "Acolyte", r"npc\pre-re\jobs\1-1\acolyte.txt"),
    ("1-1", "Merchant", r"npc\pre-re\jobs\1-1\merchant.txt"),
    ("1-1", "Thief", r"npc\pre-re\jobs\1-1\thief.txt"),
    # 2-1
    ("2-1", "Knight", r"npc\jobs\2-1\knight.txt"),
    ("2-1", "Wizard", r"npc\jobs\2-1\wizard.txt"),
    ("2-1", "Hunter", r"npc\jobs\2-1\hunter.txt"),
    ("2-1", "Priest", r"npc\jobs\2-1\priest.txt"),
    ("2-1", "Blacksmith", r"npc\jobs\2-1\blacksmith.txt"),
    ("2-1", "Assassin", r"npc\jobs\2-1\assassin.txt"),
    # 2-2
    ("2-2", "Crusader", r"npc\pre-re\jobs\2-2\crusader.txt"),
    ("2-2", "Sage", r"npc\jobs\2-2\sage.txt"),
    ("2-2", "Monk", r"npc\jobs\2-2\monk.txt"),
    ("2-2", "Rogue", r"npc\jobs\2-2\rogue.txt"),
    ("2-2", "Alchemist", r"npc\jobs\2-2\alchemist.txt"),
    ("2-2", "Bard", r"npc\jobs\2-2\bard.txt"),
    ("2-2", "Dancer", r"npc\jobs\2-2\dancer.txt"),
    # Rebirth
    ("Trans", "Valkyrie", r"npc\jobs\valkyrie.txt")
]

all_special_commands = set()
results = []

for tier, job_name, rel_path in job_files:
    full_path = os.path.join(RATHENA, rel_path)
    if not os.path.exists(full_path):
        print(f"Arquivo ausente: {full_path}")
        continue
    
    with open(full_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
        
    # Extrai o primeiro NPC principal (normalmente o líder da guilda que inicia a quest)
    # Exemplo: izlude_in,74,171,4	script	Swordsman#iz	4_M_SWORDMASTER,{
    npcs = []
    npc_pattern = re.compile(r'^([a-zA-Z0-9_]+),(\d+),(\d+),(\d+)\s+script\s+([^,\t\n{]+)\s+([0-9A-Za-z_]+)', re.M)
    for m in npc_pattern.finditer(content):
        m_map, m_x, m_y, m_dir, m_name, m_sprite = m.groups()
        npcs.append({
            "map": m_map,
            "x": int(m_x),
            "y": int(m_y),
            "dir": int(m_dir),
            "name": m_name.strip(),
            "sprite": m_sprite.strip()
        })
        
    # Comandos de script especiais
    script_commands = set()
    for cmd in ['input', 'cutin', 'viewpoint', 'progressbar', 'waitingroom', 'warp', 'misceffect', 'soundeffect', 'specialeffect', 'sc_start', 'unitwalk', 'emotion', 'getitem', 'delitem']:
        if re.search(rf'\b{cmd}\b', content):
            script_commands.add(cmd)
            all_special_commands.add(cmd)
            
    results.append({
        "tier": tier,
        "job": job_name,
        "file": rel_path,
        "main_npc": npcs[0] if npcs else None,
        "all_npcs_count": len(npcs),
        "commands": sorted(list(script_commands))
    })

print(f"Total jobs analisados: {len(results)}")
print(f"Comandos especiais encontrados no total: {sorted(list(all_special_commands))}\n")

for r in results:
    npc = r['main_npc']
    npc_str = f"{npc['name']} em {npc['map']} ({npc['x']}, {npc['y']})" if npc else "Nenhum NPC encontrado"
    print(f"[{r['tier']}] {r['job']:12s} | NPC: {npc_str}")
    print(f"      Comandos: {', '.join(r['commands'])}")
