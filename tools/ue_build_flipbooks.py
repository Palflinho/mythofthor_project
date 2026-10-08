"""
Myth of Thor - Construtor de Flipbooks Paper2D (roda DENTRO do Unreal Editor)
-----------------------------------------------------------------------------
Le o manifesto gerado por extract_act_manifest.py + a spritesheet PNG e cria:
  /Game/Sprites/<Nome>/T_<Nome>_Sheet          (textura pixel-art, filtro Nearest)
  /Game/Sprites/<Nome>/Sprites/S_<Nome>_NNN    (um PaperSprite por frame usado, pivo embaixo-centro)
  /Game/Sprites/<Nome>/Flipbooks/FB_<Nome>_<Estado>_<Dir>
  /Game/Sprites/<Nome>/DA_<Nome>               (UMythSpriteSet com 5 estados x 8 direcoes)

Uso (Output Log do editor, aba Python, ou Tools > Execute Python Script):
  py "D:/Repositorios/mythofthor_project/tools/ue_build_flipbooks.py" novice_male player "D:/Ragnarok_Assets/Sprites/PNG/초보자_남_sheet.png"
  py ".../ue_build_flipbooks.py" poring monster "D:/Ragnarok_Assets/Sprites/PNG/poring_sheet.png"

Pre-requisitos: plugins Python Editor Script e Paper2D ativos; projeto compilado (UMythSpriteSet precisa existir).
Grupos de acao do RO (action = grupo*8 + direcao):
  player : idle 0, walk 1, attack 5, hit 6, die 7
  monster: idle 0, walk 1, attack 2, hit 3, die 4
"""
import json
import os
import sys

import unreal

GROUPS = {
    'player':  {'Idle': 0, 'Walk': 1, 'Attack': 5, 'Hit': 6, 'Die': 7},
    'monster': {'Idle': 0, 'Walk': 1, 'Attack': 2, 'Hit': 3, 'Die': 4},
}
PIXELS_PER_UU = 1.0  # 1 px = 1 cm. Ajuste junto com CellSizeUU se os sprites ficarem pequenos/grandes.

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def import_sheet(png_path, folder, name):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', png_path)
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', 'T_%s_Sheet' % name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    tools.import_asset_tasks([task])
    tex = eal.load_asset('%s/T_%s_Sheet' % (folder, name))
    # Pixel art: sem mipmaps, sem filtro, sem compressao com perda.
    tex.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    return tex


def make_sprite(tex, folder, name, index, cell_w, cell_h, cols):
    asset_name = 'S_%s_%03d' % (name, index)
    path = '%s/%s' % (folder, asset_name)
    sprite = eal.load_asset(path) if eal.does_asset_exist(path) else \
        tools.create_asset(asset_name, folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
    col, row = index % cols, index // cols
    sprite.set_editor_property('source_texture', tex)
    sprite.set_editor_property('source_uv', unreal.Vector2D(col * cell_w, row * cell_h))
    sprite.set_editor_property('source_dimension', unreal.Vector2D(cell_w, cell_h))
    # A spritesheet alinha cada frame pela base e pelo centro horizontal da celula.
    sprite.set_editor_property('pivot_mode', unreal.SpritePivotMode.BOTTOM_CENTER)
    sprite.set_editor_property('pixels_per_unreal_unit', PIXELS_PER_UU)
    return sprite


def collapse(frames):
    """Frames repetidos consecutivos viram um unico key-frame com frame_run maior."""
    runs = []
    for idx in frames:
        if runs and runs[-1][0] == idx:
            runs[-1][1] += 1
        else:
            runs.append([idx, 1])
    return runs


def main(name, kind, png_path):
    manifest_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Content', 'SpriteData', '%s.json' % name)
    with open(manifest_path, encoding='utf-8') as f:
        manifest = json.load(f)

    cols = manifest['grid_columns']
    rows = -(-manifest['sprite_count'] // cols)
    root = '/Game/Sprites/%s' % name
    for sub in ('', '/Sprites', '/Flipbooks'):
        eal.make_directory(root + sub)

    tex = import_sheet(png_path, root, name)
    cell_w = tex.blueprint_get_size_x() // cols
    cell_h = tex.blueprint_get_size_y() // rows
    unreal.log('Sheet %dx%d, celula %dx%d, %d colunas' % (tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), cell_w, cell_h, cols))

    sprites = {}
    def sprite_for(idx):
        if idx not in sprites:
            sprites[idx] = make_sprite(tex, root + '/Sprites', name, idx, cell_w, cell_h, cols)
        return sprites[idx]

    data_asset_path = '%s/DA_%s' % (root, name)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MythSpriteSet)
    sprite_set = eal.load_asset(data_asset_path) if eal.does_asset_exist(data_asset_path) else \
        tools.create_asset('DA_%s' % name, root, unreal.MythSpriteSet, factory)

    for state, group in GROUPS[kind].items():
        books = []
        for direction in range(8):
            action = manifest['actions'][group * 8 + direction]
            used = [f['sprite'] for f in action['frames'] if f['sprite'] >= 0]
            fb_name = 'FB_%s_%s_%d' % (name, state, direction)
            fb_path = '%s/Flipbooks/%s' % (root, fb_name)
            fb = eal.load_asset(fb_path) if eal.does_asset_exist(fb_path) else \
                tools.create_asset(fb_name, root + '/Flipbooks', unreal.PaperFlipbook, unreal.PaperFlipbookFactory())
            keys = []
            for idx, run in collapse(used):
                kf = unreal.PaperFlipbookKeyFrame()
                kf.set_editor_property('sprite', sprite_for(idx))
                kf.set_editor_property('frame_run', run)
                keys.append(kf)
            fb.set_editor_property('key_frames', keys)
            # intervalo do .act (ms por frame) -> fps
            fb.set_editor_property('frames_per_second', 1000.0 / max(action['interval_ms'], 1.0))
            books.append(fb)
        sprite_set.set_editor_property(state.lower(), books)

    eal.save_directory(root, only_if_is_dirty=False, recursive=True)
    unreal.log('Pronto: %s (%d sprites). Aponte Sprite->SpriteSet para %s' % (name, len(sprites), data_asset_path))


if __name__ == '__main__':
    if len(sys.argv) < 4:
        unreal.log_error('Uso: ue_build_flipbooks.py <nome> <player|monster> <sheet.png>')
    else:
        main(sys.argv[1], sys.argv[2], sys.argv[3])
