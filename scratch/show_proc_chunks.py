import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open('scratch/second_class_proc_details.txt', 'r', encoding='utf-8') as f:
    text = f.read()

def show_effect(eid):
    pos = text.find(f"EFFECT ID: {eid}\n")
    if pos != -1:
        end = text.find("======================================================================", pos + 20)
        chunk = text[pos:end if end != -1 else pos + 4000]
        print(f"***** CHUNK FOR {eid} *****")
        print(chunk[:3000])

for eid in ["328", "336", "318", "319", "138", "93", "116", "253"]:
    show_effect(eid)
