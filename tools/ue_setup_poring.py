"""
Myth of Thor - Prepara o Poring como monstro de teste completo (roda DENTRO do editor / headless)
-------------------------------------------------------------------------------------------------
  1. Gera os flipbooks e o DA_poring (8 direcoes x Idle/Walk/Attack/Hit/Die) via ue_build_flipbooks.py;
  2. Importa os efeitos sonoros do Poring para /Game/Monsters/Poring/Audio (copias; os .wav originais nao mudam);
  3. Abre a Lvl_Prontera, coloca 3 AMythPoring perto do PlayerStart e salva o nivel.

O C++ (AMythPoring) procura os assets por estes caminhos, entao rode este script depois de compilar o projeto.

Uso headless:
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_setup_poring.py [--level /Game/...] [--count 3]"
Log: <projeto>/Saved/MythImport/setup_poring.log
"""
import importlib.util
import os
import sys
import traceback

import unreal

SHEET = 'D:/Ragnarok_Assets/Sprites/PNG/poring_sheet.png'
WAV_DIR = 'D:/Ragnarok_Assets/wav'
SFX = {'attack': 'poring_attack.wav', 'damage': 'poring_damage.wav', 'die': 'poring_die.wav', 'move': 'poring_move.wav'}
AUDIO_DEST = '/Game/Monsters/Poring/Audio'

_f = None


def log(msg, warn=False):
    global _f
    (unreal.log_warning if warn else unreal.log)('[Poring] ' + msg)
    if _f is None:
        d = os.path.join(unreal.Paths.project_saved_dir(), 'MythImport')
        os.makedirs(d, exist_ok=True)
        _f = open(os.path.join(d, 'setup_poring.log'), 'w', encoding='utf-8')
    _f.write(('W ' if warn else 'I ') + msg + chr(10))
    _f.flush()


def build_flipbooks():
    here = os.path.dirname(os.path.abspath(__file__))
    spec = importlib.util.spec_from_file_location('ue_build_flipbooks', os.path.join(here, 'ue_build_flipbooks.py'))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.main('poring', 'monster', SHEET)
    log('flipbooks e DA_poring gerados')


def import_sfx():
    eal = unreal.EditorAssetLibrary
    eal.make_directory(AUDIO_DEST)
    tasks = []
    for key, fn in SFX.items():
        path = WAV_DIR + '/' + fn
        if not os.path.exists(path):
            log('som nao encontrado: ' + path, warn=True)
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property('filename', path)
        t.set_editor_property('destination_path', AUDIO_DEST)
        t.set_editor_property('destination_name', 'SFX_poring_' + key)
        t.set_editor_property('replace_existing', True)
        t.set_editor_property('automated', True)
        t.set_editor_property('save', False)
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    ok = 0
    for key in SFX:
        a = eal.load_asset('%s/SFX_poring_%s' % (AUDIO_DEST, key))
        if a:
            eal.save_loaded_asset(a)
            ok += 1
        else:
            log('falha ao importar SFX_poring_' + key, warn=True)
    log('%d/%d efeitos sonoros importados' % (ok, len(SFX)))


def place_porings(level_path, count):
    lvl = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not lvl.load_level(level_path):
        log('nao foi possivel abrir ' + level_path, warn=True)
        return
    start = None
    for a in actors_sub.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            start = a
            break
    base = start.get_actor_location() if start else unreal.Vector(0, 0, 200)

    for a in actors_sub.get_all_level_actors():  # idempotente: remove os Porings de uma execucao anterior
        if isinstance(a, unreal.MythPoring):
            actors_sub.destroy_actor(a)

    for i in range(count):
        loc = unreal.Vector(base.x + 300 + 160 * i, base.y + (-150 if i % 2 == 0 else 150), base.z + 20)
        p = actors_sub.spawn_actor_from_class(unreal.MythPoring, loc, unreal.Rotator(0, 0, 0))
        if p:
            p.set_actor_label('Poring_%d' % (i + 1))
    lvl.save_current_level()
    log('%d Porings colocados perto do PlayerStart e nivel salvo' % count)


def run():
    level = '/Game/Maps/Prontera/Lvl_Prontera'
    count = 3
    argv = sys.argv[1:]
    for i, a in enumerate(argv):
        if a == '--level':
            level = argv[i + 1]
        elif a == '--count':
            count = int(argv[i + 1])
    build_flipbooks()
    import_sfx()
    place_porings(level, count)
    log('PRONTO')


try:
    run()
except Exception:  # noqa: BLE001
    log('ERRO:' + chr(10) + traceback.format_exc(), warn=True)
    raise
