# Pacote do jogo para jogar em OUTRO PC (sem Unreal instalada), apontando para o seu servidor.
#   - Compila e empacota o MythOfThor (Development, Win64) com todos os mapas (Config/DefaultGame.ini: DirectoriesToAlwaysCook)
#   - Junta as tabelas do cliente (Config/*.json) e os graficos do RO (E:\Dev\Ragnarok_Assets\grf -> RagnarokAssets\grf)
#   - Cria Jogar.bat com o endereco do servidor (-MythHost); o jogo abre na tela de login (cada um usa a sua conta)
# A conta de teste do DefaultGame.ini (NetUser/NetPassword) e retirada durante o empacotamento e volta no fim.
#
# Uso: powershell -ExecutionPolicy Bypass -File tools/make_friend_build.ps1 -ServerIp 26.x.x.x [-Out E:\Dev\MythAmigo] [-SkipAssets]
param(
    [Parameter(Mandatory = $true)][string]$ServerIp,
    [string]$Out = 'E:\Dev\MythAmigo',
    [switch]$SkipAssets
)
$ErrorActionPreference = 'Stop'
$proj = 'E:\Dev\mythofthor\MythOfThor.uproject'   # junction sem acento (o compilador nao gosta do "o" de Repositorios)
$src = 'E:\Dev\mythofthor'
$uat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$assets = 'E:\Dev\Ragnarok_Assets\grf'
$ini = Join-Path $src 'Config\DefaultGame.ini'

$abertos = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'"
if ($abertos) { 'Feche o Unreal (editor ou jogo de teste) antes: o empacotamento recompila o projeto.'; exit 3 }

# 1) Tira a conta de teste do ini (nao pode ir dentro do pacote) e devolve no fim, mesmo se der erro
$iniOriginal = [IO.File]::ReadAllText($ini)
$iniLimpo = ($iniOriginal -split "`r?`n" | Where-Object { $_ -notmatch '^\s*Net(User|Password)\s*=' }) -join "`r`n"
try {
    [IO.File]::WriteAllText($ini, $iniLimpo)
    $build = Join-Path $Out 'build'
    # So os nossos mapas (sem o conteudo do template da Unreal) e menos compiladores de shader ao mesmo tempo: com 16 GB o cook
    # completo ficava sem memoria ("Out of memory condition") e morria sem mensagem
    $maps = (Get-ChildItem (Join-Path $src 'Content\Maps') -Recurse -Filter 'Lvl_*.umap' | ForEach-Object {
        '/Game/Maps/' + $_.Directory.Name + '/' + $_.BaseName }) -join '+'
    $cookOpts = '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=10'
    & $uat BuildCookRun "-project=$proj" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$build" -utf8output -nocompileeditor "-map=$maps" "-AdditionalCookerOptions=$cookOpts"
    if ($LASTEXITCODE -ne 0) { "ERRO no empacotamento (RunUAT $LASTEXITCODE)"; exit 1 }
}
finally {
    [IO.File]::WriteAllText($ini, $iniOriginal)
}

$game = Join-Path $build 'Windows'
if (-not (Test-Path (Join-Path $game 'MythOfThor.exe'))) { "ERRO: MythOfThor.exe nao apareceu em $game"; exit 1 }

# 2) Tabelas do cliente: o jogo le Config/*.json do disco (FPaths::ProjectConfigDir)
$cfg = Join-Path $game 'MythOfThor\Config'
New-Item -ItemType Directory -Force $cfg | Out-Null
Copy-Item (Join-Path $src 'Config\*.json') $cfg -Force
robocopy (Join-Path $src 'Config\water') (Join-Path $cfg 'water') /MIR /NFL /NDL /NJH /NJS /NP | Out-Null

# 3) Graficos do RO ao lado do jogo (UMythRoSpriteComponent::GetAssetsRoot acha RagnarokAssets\grf sozinho)
if (-not $SkipAssets) {
    robocopy $assets (Join-Path $game 'RagnarokAssets\grf') /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { "ERRO copiando os graficos (robocopy $LASTEXITCODE)"; exit 1 }
}

# 4) Atalho para jogar
@"
@echo off
rem Myth of Thor - teste fechado. Precisa estar na rede da Radmin VPN do dono do servidor.
rem Se o IP do servidor mudar, troque o numero abaixo.
set SERVIDOR=$ServerIp
start "" "%~dp0MythOfThor.exe" /Game/Maps/Prontera/Lvl_Prontera -windowed -ResX=1600 -ResY=900 -MythHost=%SERVIDOR%
"@ | Set-Content (Join-Path $game 'Jogar.bat') -Encoding ASCII

$commit = (git -C $src rev-parse --short HEAD).Trim()
"Pacote gerado em $(Get-Date -Format 'yyyy-MM-dd HH:mm') a partir do commit $commit; servidor $ServerIp" | Set-Content (Join-Path $game 'VERSAO.txt') -Encoding UTF8
$tam = (Get-ChildItem -Recurse -File $game | Measure-Object Length -Sum).Sum / 1GB
"Pronto: $game ({0:N1} GB). Compacte essa pasta e mande; o amigo abre Jogar.bat." -f $tam
