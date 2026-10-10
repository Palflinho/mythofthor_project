import json

with open("Config/skill_effects_full.json", "r", encoding="utf-8") as f:
    d = json.load(f)

print(f"Effects count: {len(d.get('effects', {}))}")
print(f"Skills count: {len(d.get('skills', {}))}")
skills = d.get('skills', {})
sample = {k: skills[k] for k in list(skills.keys())[:15]}
print("Sample skills:", json.dumps(sample, indent=2))
