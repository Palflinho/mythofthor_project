import json
P='/mnt/e/Documentos/Repositórios/mythofthor_project/Config/'
fx=json.load(open(P+'skill_effects_full.json'))
tree=json.load(open(P+'skill_tree.json'))
sk=fx['skills']; ef=fx['effects']
STAGES=['effect','cast','hit','ground','success','successCaster','release','beforeHit','beforeHitCaster','hitCaster','beginCast']
def ids(e):
    out=[]
    for k in STAGES:
        v=e.get(k)
        if v is None: continue
        out+= v if isinstance(v,list) else [v]
    return out
def visual(sid):
    e=sk.get(str(sid),{})
    for i in ids(e):
        for p in ef.get(str(i),[]):
            if p.get('type') in ('STR','SPR','3D','2D','CYLINDER','QuadHorn','TRAIL','RSM','WATERFALL'): return 'visual'
    return 'audio' if ids(e) else 'nenhum'
jobs=[j for j in tree['jobs']]
names={}
for sid,s in tree['skills'].items(): names[int(sid)]=s['name']
# so skills de classes ate transclasse: ids de job 0..25 e 4001..4049
scope=[j for j in jobs if (j.isdigit() and (int(j)<=25 or 4001<=int(j)<=4049))]
seen={}
for j in scope:
    for cell,sid in tree['jobs'][j].items():
        seen.setdefault(int(sid),j)
cnt={'visual':0,'audio':0,'nenhum':0}
miss=[]
for sid in sorted(seen):
    v=visual(sid); cnt[v]+=1
    if v!='visual': miss.append((sid,names.get(sid,'?'),v))
print(cnt)
print(len(miss))
for m in miss: print(m)
import re,os
act=set(); cur=None
for line in open(os.path.expanduser('~/mythofthor-server/rathena/db/re/skill_db.yml'),encoding='utf-8',errors='replace'):
    m=re.match(r'^\s*-\s*Id:\s*(\d+)',line)
    if m: cur=int(m.group(1)); continue
    m=re.match(r'^\s{4}TargetType:\s*(\w+)',line)
    if m and cur: act.add(cur)
print('--- ATIVAS sem visual ---')
for sid,n,v in miss:
    if sid in act and v=='nenhum': print(sid,n)
print('--- ATIVAS so audio ---')
print([ (sid,n) for sid,n,v in miss if sid in act and v=='audio'])
