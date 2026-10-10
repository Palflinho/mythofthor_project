#!/usr/bin/env python3
"""
Gera Config/skill_tree.json a partir dos .lub do cliente LATAM (via tools/lub_eval.py):
  skilltreeview.lub -> posicao de cada skill na grade da janela (indice = linha * 7 + coluna), por classe
  skillinfolist.lub -> nome (pt-BR no cliente LATAM), nivel maximo, SP por nivel, alcance e pre-requisitos

Saida: {"jobs": {jobId: {indice: skillId}}, "skills": {skillId: {"name", "aegis", "maxlv", "sp": [...], "need": [[id, lv]],
        "need_by_job": {jobId: [[id, lv]]}}}}

Uso: python3 tools/gen_skill_tree.py <pasta "lua files" do GRF>
"""
import json
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EVAL = os.path.join(ROOT, 'tools', 'lub_eval.py')


def evaluate(files):
    with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as tmp:
        out = tmp.name
    subprocess.run([sys.executable, EVAL, out] + files, check=True, stdout=subprocess.DEVNULL)
    with open(out, encoding='utf-8') as fh:
        data = json.load(fh)
    os.remove(out)
    return data


def pairs(node):
    """{"1": {"1": id, "2": lv}, ...} -> [[id, lv], ...]"""
    result = []
    if isinstance(node, dict):
        for _, v in sorted(node.items(), key=lambda kv: int(kv[0]) if kv[0].lstrip('-').isdigit() else 0):
            if isinstance(v, dict) and '1' in v and '2' in v:
                result.append([v['1'], v['2']])
    return result


def main():
    lua = sys.argv[1]
    base = [os.path.join(lua, 'skillinfoz', 'skillid.lub'), os.path.join(lua, 'datainfo', 'jobidentity.lub')]
    tree = evaluate(base + [os.path.join(lua, 'skillinfoz', 'skilltreeview.lub')])
    info = evaluate(base + [os.path.join(lua, 'skillinfoz', 'skillinfolist.lub')])

    jobs = {}
    for job, grid in tree.get('SKILL_TREEVIEW_FOR_JOB', {}).items():
        if isinstance(grid, dict) and job.lstrip('-').isdigit():  # classes sem JT_ no jobidentity ficam de fora
            jobs[job] = {k: v for k, v in grid.items() if isinstance(v, int)}

    used = {sid for grid in jobs.values() for sid in grid.values()}
    skills = {}
    for sid, entry in info.get('SKILL_INFO_LIST', {}).items():
        if not isinstance(entry, dict) or int(sid) not in used:
            continue
        sp = entry.get('SpAmount') or {}
        need_by_job = {}
        nsl = entry.get('NeedSkillList')
        if isinstance(nsl, dict):
            for job, lst in nsl.items():
                need_by_job[job] = pairs(lst)
        skills[sid] = {
            'name': entry.get('SkillName', ''),
            'aegis': entry.get('1', ''),
            'maxlv': entry.get('MaxLv', 1),
            'sp': [sp[k] for k in sorted(sp, key=int)] if isinstance(sp, dict) else [],
            'need': pairs(entry.get('_NeedSkillList')),
            'need_by_job': need_by_job,
        }

    out = os.path.join(ROOT, 'Config', 'skill_tree.json')
    with open(out, 'w', encoding='utf-8') as fh:
        json.dump({'jobs': jobs, 'skills': skills}, fh, ensure_ascii=False, indent=1)
    print('%d classes, %d skills em %s' % (len(jobs), len(skills), out))


if __name__ == '__main__':
    main()
