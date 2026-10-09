#!/usr/bin/env python3
"""
Myth of Thor - Client Data Tables Extractor
------------------------------------------
Extrai e converte as tabelas de dados do cliente Ragnarok Online para JSON em Config/:
1. Config/items.json    - Itens (ID, nome PT-BR, nome de recurso/ícone, slots, classe, descrição)
2. Config/skills.json   - Habilidades (ID numérico rAthena, AegisName, nome PT-BR, descrição)
3. Config/maps.json     - Nomes dos mapas (mapnametable.txt convertido para UTF-8)
4. Config/messages.json - Mensagens do sistema (msgstringtable_ml.csv convertido para UTF-8)
"""

import os
import sys
import re
import json
import base64
from typing import Dict, List, Any, Optional

try:
    from tools.grf_extract import GrfArchive
except ImportError:
    from grf_extract import GrfArchive


def extract_items(lua_path: str, out_json: str):
    print(f"[1/4] Extraindo itens de: {lua_path}")
    if not os.path.exists(lua_path):
        print(f"  [AVISO] Arquivo {lua_path} não encontrado!")
        return

    with open(lua_path, 'r', encoding='cp1252', errors='ignore') as f:
        content = f.read()

    raw_parts = re.split(r'\n\t*\[(\d+)\]\s*=\s*\{', content)
    print(f"  Blocos de itens encontrados no Lua: {len(raw_parts) // 2:,}")

    items: Dict[str, Any] = {}

    for idx in range(1, len(raw_parts), 2):
        iid_str = raw_parts[idx]
        block = raw_parts[idx + 1]
        iid = int(iid_str)

        name_m = re.search(r'identifiedDisplayName\s*=\s*\[=\[(.*?)\]=\]', block)
        name = name_m.group(1).strip() if name_m else ''

        res_m = re.search(r'identifiedResourceName\s*=\s*\[=\[(.*?)\]=\]', block)
        res_raw = res_m.group(1).strip() if res_m else ''

        # Converte mojibake CP949 se necessário
        res_korean = res_raw
        if res_raw:
            try:
                res_korean = res_raw.encode('latin1').decode('cp949')
            except Exception:
                pass

        slots_m = re.search(r'slotCount\s*=\s*(\d+)', block)
        slots = int(slots_m.group(1)) if slots_m else 0

        class_m = re.search(r'ClassNum\s*=\s*(\d+)', block)
        class_num = int(class_m.group(1)) if class_m else 0

        # Extrai linhas de descrição de identifiedDescriptionName
        desc_block_m = re.search(r'identifiedDescriptionName\s*=\s*\{(.*?)\}', block, re.DOTALL)
        desc_lines = []
        if desc_block_m:
            desc_lines = [
                d.strip() for d in re.findall(r'\[=\[(.*?)\]=\]', desc_block_m.group(1))
            ]

        items[str(iid)] = {
            "id": iid,
            "name": name,
            "resource_name": res_korean,
            "slots": slots,
            "class_num": class_num,
            "description": desc_lines
        }

    os.makedirs(os.path.dirname(out_json), exist_ok=True)
    with open(out_json, 'w', encoding='utf-8') as f:
        json.dump(items, f, indent=2, ensure_ascii=False)

    print(f"  -> Salvo com sucesso: {out_json} ({len(items):,} itens)")


def extract_skills(grf: GrfArchive, skill_db_yml: str, out_json: str):
    print(f"[2/4] Extraindo habilidades do GRF e mapeando com skill_db.yml...")

    # 1. Carrega mapeamento AegisName -> ID do skill_db.yml do rAthena
    aegis_to_id: Dict[str, int] = {}
    if os.path.exists(skill_db_yml):
        with open(skill_db_yml, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            cur_id = 0
            for line in lines:
                id_m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
                if id_m:
                    cur_id = int(id_m.group(1))
                    continue
                name_m = re.match(r'^\s*Name:\s*([A-Za-z0-9_]+)', line)
                if name_m and cur_id > 0:
                    aegis_to_id[name_m.group(1)] = cur_id
                    cur_id = 0
        print(f"  Mapeamento de skills rAthena carregado: {len(aegis_to_id):,} skills")

    # 2. Carrega nomes de skillnametable.txt
    name_entry = grf.get_entry(r"data\skillnametable.txt")
    skill_names: Dict[str, str] = {}
    if name_entry:
        raw = grf.extract_entry(name_entry).decode('cp1252', errors='ignore')
        for line in raw.splitlines():
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            parts = [p for p in line.split('#') if p]
            if len(parts) >= 2:
                aegis = parts[0].strip()
                disp_name = parts[1].replace('_', ' ').strip()
                skill_names[aegis] = disp_name

    # 3. Carrega descrições de skilldesctable.txt
    desc_entry = grf.get_entry(r"data\skilldesctable.txt")
    skill_descs: Dict[str, List[str]] = {}
    if desc_entry:
        raw = grf.extract_entry(desc_entry).decode('cp1252', errors='ignore')
        current_aegis = ""
        current_lines = []
        for line in raw.splitlines():
            line_s = line.strip()
            if line_s.endswith('#') and len(line_s) > 1 and not line_s.startswith('//'):
                if current_aegis and current_lines:
                    skill_descs[current_aegis] = current_lines
                current_aegis = line_s[:-1].strip()
                current_lines = []
            elif line_s == '#':
                if current_aegis and current_lines:
                    skill_descs[current_aegis] = current_lines
                current_aegis = ""
                current_lines = []
            else:
                if current_aegis and line:
                    current_lines.append(line)
        if current_aegis and current_lines:
            skill_descs[current_aegis] = current_lines

    # Combina tudo em uma tabela unificada
    skills: Dict[str, Any] = {}
    all_aegis = set(skill_names.keys()) | set(skill_descs.keys()) | set(aegis_to_id.keys())

    for aegis in sorted(all_aegis):
        sid = aegis_to_id.get(aegis, 0)
        name = skill_names.get(aegis, aegis)
        desc = skill_descs.get(aegis, [])

        skill_obj = {
            "id": sid,
            "aegis_name": aegis,
            "name": name,
            "description": desc
        }
        # Indexa por ID numérico (como string) e por AegisName
        if sid > 0:
            skills[str(sid)] = skill_obj
        skills[aegis] = skill_obj

    os.makedirs(os.path.dirname(out_json), exist_ok=True)
    with open(out_json, 'w', encoding='utf-8') as f:
        json.dump(skills, f, indent=2, ensure_ascii=False)

    print(f"  -> Salvo com sucesso: {out_json} ({len(skills):,} entradas indexadas)")


def extract_maps(grf: GrfArchive, out_json: str):
    print(f"[3/4] Extraindo nomes dos mapas de mapnametable.txt...")
    entry = grf.get_entry(r"data\mapnametable.txt")
    if not entry:
        print("  [AVISO] data\\mapnametable.txt não encontrado no GRF!")
        return

    raw = grf.extract_entry(entry).decode('cp1252', errors='ignore')
    maps: Dict[str, Any] = {}

    for line in raw.splitlines():
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        parts = [p.strip() for p in line.split('#') if p.strip()]
        if len(parts) >= 2:
            rsw_file = parts[0]
            display_name = parts[1]
            map_key = rsw_file.lower().replace('.rsw', '')

            maps[map_key] = {
                "map_id": map_key,
                "file": rsw_file,
                "name": display_name
            }

    os.makedirs(os.path.dirname(out_json), exist_ok=True)
    with open(out_json, 'w', encoding='utf-8') as f:
        json.dump(maps, f, indent=2, ensure_ascii=False)

    print(f"  -> Salvo com sucesso: {out_json} ({len(maps):,} mapas mapeados)")


def extract_messages(grf: GrfArchive, out_json: str):
    print(f"[4/4] Extraindo mensagens do sistema de msgstringtable_ml.csv...")
    entry = grf.get_entry(r"data\msgstringtable_ml.csv")
    if not entry:
        print("  [AVISO] data\\msgstringtable_ml.csv não encontrado no GRF!")
        return

    raw = grf.extract_entry(entry).decode('utf-8', errors='ignore')
    messages: Dict[str, Any] = {}

    for idx, line in enumerate(raw.splitlines()):
        cols = line.split(',')
        if not cols:
            continue
        key = base64.b64decode(cols[0]).decode('utf-8', errors='ignore') if cols[0] else f"MSG_{idx}"
        en = base64.b64decode(cols[2]).decode('utf-8', errors='ignore') if len(cols) > 2 and cols[2] else ""
        pt = ""
        if len(cols) > 7 and cols[7]:
            try:
                pt = base64.b64decode(cols[7]).decode('utf-8', errors='ignore')
            except Exception:
                pass
        if not pt:
            pt = en

        msg_obj = {
            "index": idx,
            "key": key,
            "text": pt,
            "text_en": en
        }
        messages[str(idx)] = msg_obj
        messages[key] = msg_obj

    os.makedirs(os.path.dirname(out_json), exist_ok=True)
    with open(out_json, 'w', encoding='utf-8') as f:
        json.dump(messages, f, indent=2, ensure_ascii=False)

    print(f"  -> Salvo com sucesso: {out_json} ({len(messages):,} mensagens indexadas)")


def main():
    grf_path = r"C:\Gravity\Ragnarok\data.grf"
    item_lua_path = r"C:\Gravity\Ragnarok\System\itemInfo.lua"
    skill_db_path = r"server\rathena\db\pre-re\skill_db.yml"

    # Se skill_db_path relativo não existir, tenta caminhos conhecidos do WSL
    if not os.path.exists(skill_db_path):
        wsl_candidates = [
            r"\\wsl$\Ubuntu\home\palf\mythofthor-server\rathena\db\pre-re\skill_db.yml",
            r"\\wsl.localhost\Ubuntu\home\palf\mythofthor-server\rathena\db\pre-re\skill_db.yml"
        ]
        for c in wsl_candidates:
            if os.path.exists(c):
                skill_db_path = c
                break

    print("=" * 60)
    print("MYTH OF THOR - EXTRATOR DE TABELAS DE DADOS DO CLIENTE")
    print("=" * 60)

    grf = GrfArchive(grf_path)

    extract_items(item_lua_path, r"Config\items.json")
    extract_skills(grf, skill_db_path, r"Config\skills.json")
    extract_maps(grf, r"Config\maps.json")
    extract_messages(grf, r"Config\messages.json")

    print("=" * 60)
    print("TODAS AS TABELAS FORAM GERADAS COM SUCESSO EM Config/! ✅")
    print("=" * 60)


if __name__ == "__main__":
    main()
