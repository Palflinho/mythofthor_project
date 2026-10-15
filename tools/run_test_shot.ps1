param(
    [string]$Map = '/Game/Maps/Prontera/Lvl_Prontera',
    [string]$ShotName = 'test_gradient_prt',
    [string]$Warp = '',
    [float]$Delay = 18.0,
    [float]$Hour = 12.0,
    [int]$QuitAfter = 24
)

$ue = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$proj = 'E:\Dev\mythofthor\mythofthor.uproject'

$cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123"
if ($Warp -ne '') {
    $w = $Warp.Replace(' ', '+')
    $cmds += ", MythDelayed 10 MythSay+@warp+$w"
}
$cmds += ", MythDelayed $Delay MythShot+0+$Hour+$ShotName"
$cmds += ", MythDelayed $QuitAfter quit"

$argsList = @($proj, $Map, '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoLiveCoding', "-ExecCmds=$cmds")

Write-Host "Iniciando teste standalone no mapa $Map com print '$ShotName' (Warp: '$Warp')..."
$p = Start-Process -FilePath $ue -ArgumentList $argsList -PassThru
Write-Host "Processo UnrealEditor PID: $($p.Id)"
$finished = $p.WaitForExit(180000)
if (-not $finished) {
    Write-Host "Tempo limite de 180s atingido, finalizando processo..."
    $p.Kill()
} else {
    Write-Host "Processo concluido com codigo $($p.ExitCode)."
}
