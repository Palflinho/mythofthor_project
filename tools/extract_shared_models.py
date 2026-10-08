import os
import subprocess
import time

grfcl_dir = r"D:\Downloads\GrfCL"
grfcl_exe = os.path.join(grfcl_dir, "GrfCL.exe")
grf_file = r"D:\Downloads\data.grf"
dest_dir = r"D:\Ragnarok_Assets\shared"
os.makedirs(dest_dir, exist_ok=True)

print("Iniciando extracao de data\\model...")
start = time.time()
cmd = [
    grfcl_exe,
    "-breakOnExceptions", "false",
    "-encoding", "949",
    "-open", grf_file,
    "-extractFiles", r"data\model", dest_dir
]
p = subprocess.Popen(cmd, cwd=grfcl_dir, creationflags=0x00000010)
p.wait()
dur = time.time() - start
print(f"Extracao concluida em {dur:.1f}s com retorno {p.returncode}!")

model_count = 0
for root, _, files in os.walk(dest_dir):
    model_count += len([f for f in files if f.endswith('.rsm')])
print(f"Modelos RSM extraidos em {dest_dir}: {model_count}")
