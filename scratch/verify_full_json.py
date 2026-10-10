import json

with open("Config/skill_effects_full.json", "r", encoding="utf-8") as f:
    d = json.load(f)

print(f"Total skills: {len(d['skills'])}")
print(f"Total effects: {len(d['effects'])}")

for sid in ['56', '70', '79', '84', '89', '116', '136', '254', '271']:
    print(f"Skill {sid}: {d['skills'].get(sid)}")

for eid in ['226', '328', '336', '318', '319', '2006', '2007', '2008']:
    parts = d['effects'].get(eid, [])
    print(f"Effect {eid}: {len(parts)} parts")
