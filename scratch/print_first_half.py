import os, re, json
from build_second_class_map import jobs, consts, sk_effects, elements

first_half = ["Knight", "Priest", "Wizard", "Blacksmith", "Hunter", "Assassin", "Crusader"]
for job in first_half:
    skill_list = jobs[job]
    print(f"=== {job} ({len(skill_list)} skills) ===")
    for sk in skill_list:
        sid = consts.get(sk)
        eff = sk_effects.get(sk, "{}")
        elem = elements.get(sid, "Neutral")
        print(f"  {sid:4d} | {sk:20s} | Element: {elem:7s} | Effect: {eff}")
