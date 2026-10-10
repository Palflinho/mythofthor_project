import json

with open("scratch/second_class_recipes_summary.json", "r", encoding="utf-8") as f:
    data = json.load(f)

procedural_types = {"CYLINDER", "3D", "2D", "FUNC"}
proc_effects = {}

for eid, parts in data.items():
    proc_parts = [p for p in parts if p.get("type") in procedural_types]
    if proc_parts:
        proc_effects[eid] = proc_parts

print(f"Total efeitos procedurais na 2ª classe: {len(proc_effects)}")
for eid, parts in sorted(proc_effects.items(), key=lambda x: (int(x[0]) if x[0].isdigit() else 9999, x[0])):
    types_str = ", ".join(p['type'] for p in parts)
    files_str = ", ".join(p.get('file', '') for p in parts if p.get('file'))
    print(f"  Efeito {eid:15s} | Types: [{types_str}] | Files: [{files_str}]")
