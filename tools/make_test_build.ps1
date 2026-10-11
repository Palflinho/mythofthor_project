# Cria (ou atualiza) a "versao de teste" do jogo, uma copia congelada do projeto compilado, para o usuario testar sem
# atrapalhar quem continua mexendo no projeto. Fica em E:\Dev\MythTeste (sem acento: o compilador nao gosta).
# Uso: powershell -ExecutionPolicy Bypass -File tools/make_test_build.ps1 [-Nota "o que mudou"]
# Recusa copiar se a janela de teste estiver aberta (a DLL fica presa).
param([string]$Nota = '')
$src = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dst = 'E:\Dev\MythTeste'
$testando = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" | Where-Object { $_.CommandLine -like '*MythTeste*' }
if ($testando) { 'A VERSAO DE TESTE ESTA ABERTA: feche o jogo e rode de novo.'; exit 3 }
New-Item -ItemType Directory -Force $dst | Out-Null
# Binaries (DLL do jogo), Config (ini + json das tabelas) e Content (mapas e assets). Sem Source, Intermediate nem Saved.
foreach ($pasta in 'Binaries', 'Config', 'Content') {
    robocopy (Join-Path $src $pasta) (Join-Path $dst $pasta) /MIR /NFL /NDL /NJH /NJS /NP /XD Intermediate Saved | Out-Null
    if ($LASTEXITCODE -ge 8) { "ERRO copiando $pasta (robocopy $LASTEXITCODE)"; exit 1 }
}
Copy-Item (Join-Path $src 'mythofthor.uproject') $dst -Force
$commit = (git -C $src rev-parse --short HEAD).Trim()
$data = Get-Date -Format 'yyyy-MM-dd HH:mm'
"Versao de teste gerada em $data a partir do commit $commit`r`n$Nota" | Set-Content (Join-Path $dst 'VERSAO.txt') -Encoding UTF8
@'
@echo off
rem Myth of Thor - VERSAO DE TESTE (copia congelada; nao muda enquanto o desenvolvimento continua)
rem Abra o servidor (Docker/WSL) antes. Conta de teste: mythtest. Detalhes em COMO_TESTAR.txt
set UE="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set PROJ="E:\Dev\MythTeste\mythofthor.uproject"
start "" %UE% %PROJ% /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1600 -ResY=900 -log -NoLiveCoding -MythOnline
'@ | Set-Content (Join-Path $dst 'Jogar_Myth_Teste.bat') -Encoding ASCII
@'
@echo off
rem Myth of Thor - VERSAO DE TESTE com a tela de login (nao entra sozinho). Use uma conta criada no servidor.
set UE="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set PROJ="E:\Dev\MythTeste\mythofthor.uproject"
start "" %UE% %PROJ% /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1600 -ResY=900 -log -NoLiveCoding
'@ | Set-Content (Join-Path $dst 'Jogar_Myth_Teste_Com_Login.bat') -Encoding ASCII
Copy-Item (Join-Path $src 'docs\TESTE_JOGADOR.md') (Join-Path $dst 'COMO_TESTAR.txt') -Force -ErrorAction SilentlyContinue
"Pronto: $dst (commit $commit). Abra Jogar_Myth_Teste.bat."
