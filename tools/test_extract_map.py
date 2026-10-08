import os
import subprocess

grfcl_dir = r"D:\Downloads\GrfCL"
grfcl_exe = os.path.join(grfcl_dir, "GrfCL.exe")
grf_file = r"D:\Downloads\data.grf"
dest_dir = r"D:\Ragnarok_Assets\Izlude\Izlude_Cidade"
os.makedirs(dest_dir, exist_ok=True)

targets = [
    r"data\izlude.rsw",
    r"data\izlude.gnd",
    r"data\izlude.gat"
]

for t in targets:
    cmd = [
        grfcl_exe,
        "-breakOnExceptions", "false",
        "-encoding", "949",
        "-open", grf_file,
        "-extractFiles", t, dest_dir
    ]
    p = subprocess.Popen(cmd, cwd=grfcl_dir, creationflags=0x00000010)
    p.wait()
    print(f"Executado para {t}: returncode={p.returncode}")

print("Arquivos extraidos:")
for root, _, files in os.walk(dest_dir):
    for f in files:
        fp = os.path.join(root, f)
        print(f"  {f} ({os.path.getsize(fp)} bytes)")
