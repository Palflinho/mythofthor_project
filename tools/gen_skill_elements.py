#!/usr/bin/env python3
"""
gen_skill_elements.py - Generates Config/skill_elements.json from rAthena skill_db.yml.
Maps every skill ID to its AegisName, canonical element, and HD-2D dynamic light color.
"""
import os
import sys
import json
import re

RATHENA_PATH = os.path.expanduser("~/mythofthor-server/rathena/db/re/skill_db.yml")
OUTPUT_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Config", "skill_elements.json")

# GDD HD-2D elemental palette
ELEMENT_COLORS = {
    "Fire":     {"hex": "#FF5500", "r": 1.0,  "g": 0.35, "b": 0.02, "name": "Fogo / Laranja"},
    "Water":    {"hex": "#0099FF", "r": 0.05, "g": 0.55, "b": 1.0,  "name": "Agua / Azul"},
    "Wind":     {"hex": "#00F0FF", "r": 0.1,  "g": 0.9,  "b": 0.95, "name": "Vento / Ciano"},
    "Earth":    {"hex": "#CD853F", "r": 0.85, "g": 0.52, "b": 0.22, "name": "Terra / Ocre"},
    "Holy":     {"hex": "#FFF066", "r": 1.0,  "g": 0.92, "b": 0.38, "name": "Sagrado / Dourado"},
    "Dark":     {"hex": "#8A2BE2", "r": 0.58, "g": 0.15, "b": 0.92, "name": "Sombrio / Violeta"},
    "Shadow":   {"hex": "#8A2BE2", "r": 0.58, "g": 0.15, "b": 0.92, "name": "Sombrio / Violeta"},
    "Ghost":    {"hex": "#708090", "r": 0.45, "g": 0.65, "b": 0.95, "name": "Fantasma / Etereo"},
    "Poison":   {"hex": "#32CD32", "r": 0.2,  "g": 0.85, "b": 0.1,  "name": "Veneno / Verde"},
    "Undead":   {"hex": "#8B0000", "r": 0.65, "g": 0.05, "b": 0.35, "name": "Morto-Vivo / Carmesim"},
    "Neutral":  {"hex": "#E0E0FF", "r": 0.88, "g": 0.88, "b": 0.95, "name": "Neutro / Branco"},
    "Weapon":   {"hex": "#FFF5EE", "r": 0.95, "g": 0.90, "b": 0.82, "name": "Arma / Neutro"},
}

def parse_skill_db(path):
    skills = {}
    if not os.path.isfile(path):
        print(f"Error: {path} not found.")
        return skills

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    current_id = None
    current_name = None
    current_element = "Neutral"

    re_id = re.compile(r"^\s*-\s*Id:\s*(\d+)")
    re_name = re.compile(r"^\s*Name:\s*([A-Za-z0-9_]+)")
    re_elem_single = re.compile(r"^\s*Element:\s*([A-Za-z0-9_]+)")
    re_elem_level = re.compile(r"^\s*Element:\s*([A-Za-z0-9_]+)")

    for line in lines:
        m_id = re_id.match(line)
        if m_id:
            if current_id is not None:
                elem_info = ELEMENT_COLORS.get(current_element, ELEMENT_COLORS["Neutral"])
                skills[str(current_id)] = {
                    "id": current_id,
                    "name": current_name or f"SKILL_{current_id}",
                    "element": current_element,
                    "color": [elem_info["r"], elem_info["g"], elem_info["b"]],
                    "hex": elem_info["hex"]
                }
            current_id = int(m_id.group(1))
            current_name = None
            current_element = "Neutral"
            continue

        if current_id is None:
            continue

        m_name = re_name.match(line)
        if m_name:
            current_name = m_name.group(1)
            continue

        m_elem = re_elem_single.match(line)
        if m_elem:
            elem_str = m_elem.group(1)
            if elem_str in ELEMENT_COLORS:
                current_element = elem_str
            elif elem_str.capitalize() in ELEMENT_COLORS:
                current_element = elem_str.capitalize()
            continue

    if current_id is not None:
        elem_info = ELEMENT_COLORS.get(current_element, ELEMENT_COLORS["Neutral"])
        skills[str(current_id)] = {
            "id": current_id,
            "name": current_name or f"SKILL_{current_id}",
            "element": current_element,
            "color": [elem_info["r"], elem_info["g"], elem_info["b"]],
            "hex": elem_info["hex"]
        }

    return skills

def main():
    print(f"Reading rAthena skill_db from {RATHENA_PATH}...")
    skills = parse_skill_db(RATHENA_PATH)
    print(f"Parsed {len(skills)} skills from skill_db.yml.")

    # Also augment with Config/skills.json if available
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    config_skills_path = os.path.join(project_dir, "Config", "skills.json")
    if os.path.isfile(config_skills_path):
        try:
            with open(config_skills_path, "r", encoding="utf-8", errors="ignore") as f:
                c_data = json.load(f)
            for k, item in c_data.items():
                if isinstance(item, dict) and "id" in item:
                    sid = str(item["id"])
                    if sid not in skills:
                        elem = "Neutral"
                        # Search description for "Element: ...Holy..."
                        desc_str = " ".join(item.get("description", []))
                        for cand in ["Fire", "Water", "Wind", "Earth", "Holy", "Dark", "Shadow", "Ghost", "Poison", "Undead"]:
                            if f"Element: ^{cand}" in desc_str or f"^{cand}" in desc_str or cand in desc_str:
                                elem = cand
                                break
                        elem_info = ELEMENT_COLORS.get(elem, ELEMENT_COLORS["Neutral"])
                        skills[sid] = {
                            "id": item["id"],
                            "name": item.get("aegis_name", f"SKILL_{sid}"),
                            "element": elem,
                            "color": [elem_info["r"], elem_info["g"], elem_info["b"]],
                            "hex": elem_info["hex"]
                        }
        except Exception as e:
            print(f"Warning parsing skills.json: {e}")

    out_file = os.path.join(project_dir, "Config", "skill_elements.json")
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump(skills, f, indent=2)

    print(f"Successfully generated {out_file} with {len(skills)} skills.")

if __name__ == "__main__":
    main()
