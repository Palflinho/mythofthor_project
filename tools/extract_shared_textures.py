import os
import subprocess
import time

grfcl_dir = r"D:\Downloads\GrfCL"
grfcl_exe = os.path.join(grfcl_dir, "GrfCL.exe")
grf_file = r"D:\Downloads\data.grf"
dest_dir = r"D:\Ragnarok_Assets\shared"

print("Iniciando extracao de data\\texture...")
start = time.time()
cmd = [
    grfcl_exe,
    "-breakOnExceptions", "false",
    "-encoding", "949",
    "-open", grf_file,
    "-extractFiles", r"data\texture", dest_dir
]
p = subprocess.Popen(cmd, cwd=grfcl_dir, creationflags=0x00000010)
p.wait()
dur = time.time() - start
print(f"Extracao de texturas concluida em {dur:.1f}s com retorno {p.returncode}!")

tex_count = 0
for root, _, files in os.walk(os.path.join(dest_dir, "data", "texture")):
    tex_count += len([f for f in files if f.lower().endswith('.bmp') or f.lower().endswith('.tga')])
print(f"Texturas extraidas: {tex_count}")
