import os
import struct
import json

if os.name == 'nt':
    BASE_DIR = r"E:\Dev\Ragnarok_Assets\grf\data\texture\유저인터페이스"
else:
    BASE_DIR = "/mnt/e/Dev/Ragnarok_Assets/grf/data/texture/유저인터페이스"

def get_image_info(filepath):
    try:
        with open(filepath, "rb") as f:
            header = f.read(32)
            if header[:2] == b"BM":
                w, h = struct.unpack_from("<ii", header, 18)
                bpp, = struct.unpack_from("<H", header, 28)
                return w, abs(h), bpp, "BMP"
            elif len(header) >= 18:
                # Check TGA
                w, h = struct.unpack_from("<HH", header, 12)
                bpp = header[16]
                if w > 0 and h > 0 and bpp in (8, 16, 24, 32):
                    return w, h, bpp, "TGA"
    except Exception as e:
        pass
    return None

def analyze_folder(folder_name, filter_keywords=None):
    folder_path = os.path.join(BASE_DIR, folder_name)
    if not os.path.exists(folder_path):
        return []
    
    results = []
    for root, dirs, files in os.walk(folder_path):
        for fname in sorted(files):
            ext = os.path.splitext(fname)[1].lower()
            if ext in ('.bmp', '.tga'):
                fpath = os.path.join(root, fname)
                info = get_image_info(fpath)
                rel_path = os.path.relpath(fpath, BASE_DIR)
                if filter_keywords:
                    match = any(kw.lower() in fname.lower() for kw in filter_keywords)
                    if not match: continue
                if info:
                    w, h, bpp, fmt = info
                    results.append({
                        "file": fname,
                        "rel_path": rel_path.replace("\\", "/"),
                        "width": w,
                        "height": h,
                        "bpp": bpp,
                        "size_bytes": os.path.getsize(fpath)
                    })
    return results

def main():
    print("=== INSPEÇÃO DE TEXTURAS DA HUD MODERNA LATAM ===")
    
    # 1. Dialog / NPC Talk
    print("\n--- 1. DIALOG / NPC TALK ---")
    npc_dialog_files = analyze_folder("basic_interface", ["dia", "talk", "npc", "btn_close", "btn_ok", "btn_next", "btn_prev", "chat", "msg", "scroll", "bar", "box"])
    for f in npc_dialog_files[:30]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 2. Inventory
    print("\n--- 2. INVENTORY ---")
    inv_files = analyze_folder("inventory")
    for f in inv_files[:25]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 3. StatusWnd
    print("\n--- 3. STATUS WND ---")
    status_files = analyze_folder("statuswnd")
    for f in status_files[:25]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 4. Shortcut / Hotkey
    print("\n--- 4. SHORTCUT ---")
    sc_files = analyze_folder("shortcut")
    for f in sc_files[:20]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 5. Minimap
    print("\n--- 5. MINIMAP ---")
    mm_files = analyze_folder("minimap")
    for f in mm_files[:20]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 6. GameSettingsUI / ESC Menu
    print("\n--- 6. GAMESETTINGSUI ---")
    gs_files = analyze_folder("gamesettingsui")
    for f in gs_files[:25]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 7. RenewalParty
    print("\n--- 7. RENEWALPARTY ---")
    party_files = analyze_folder("renewalparty")
    for f in party_files[:20]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

    # 8. Grade Enchant
    print("\n--- 8. GRADE ENCHANT ---")
    grade_files = analyze_folder("grade_enchant")
    for f in grade_files[:20]:
        print(f"[{f['rel_path']}] {f['width']}x{f['height']} ({f['bpp']}bpp, {f['size_bytes']}B)")

if __name__ == '__main__':
    main()
