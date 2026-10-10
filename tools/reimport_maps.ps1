param(
    [string[]]$MapNames = @('prt_fild06', 'prt_fild08', 'prt_in', 'prt_church', 'prt_castle')
)

$lock = "E:\Dev\mythofthor\coord\build.lock"
$ueCmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$uproject = "E:\Dev\mythofthor\mythofthor.uproject"
$script = "E:/Dev/mythofthor/tools/ue_import_map.py"

$mapConfigs = @{
    'prontera' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/Prontera_Cidade/prontera_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/08.mp3 --start-cell 156,191'
    }
    'prt_fild05' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_fild05/prt_fild05_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/12.mp3'
    }
    'prt_fild06' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_fild06/prt_fild06_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/12.mp3'
    }
    'prt_fild08' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_fild08/prt_fild08_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/12.mp3'
    }
    'prt_in' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_in/prt_in_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/210.mp3'
    }
    'prt_church' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_church/prt_church_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/10.mp3'
    }
    'prt_castle' = @{
        'obj' = 'E:/Dev/Ragnarok_Assets/Prontera/prt_castle/prt_castle_full.obj'
        'extra' = '--bgm E:/Dev/Ragnarok_Assets/BGM/09.mp3'
    }
}

try {
    "antigravity $(Get-Date -Format s)" | Set-Content $lock -Encoding UTF8
    Write-Host "Lock criado em $lock"

    foreach ($m in $MapNames) {
        if (-not $mapConfigs.ContainsKey($m)) {
            Write-Warning "Mapa desconhecido: $m"
            continue
        }
        $cfg = $mapConfigs[$m]
        $obj = $cfg['obj']
        $extra = $cfg['extra']
        $scriptArg = "-script=`"$script $obj $extra`""
        Write-Host "=========================================="
        Write-Host "Reimportando $m..."
        Write-Host "Cmd: $ueCmd $uproject -run=pythonscript $scriptArg"
        Write-Host "=========================================="
        
        $proc = Start-Process -FilePath $ueCmd -ArgumentList @($uproject, '-run=pythonscript', $scriptArg, '-unattended', '-nopause') -Wait -PassThru -NoNewWindow
        Write-Host "Finalizado $m com exit code $($proc.ExitCode)"
        if ($proc.ExitCode -ne 0) {
            Write-Error "Falha ao importar $m (exit code $($proc.ExitCode))"
        }
    }
} finally {
    if (Test-Path $lock) {
        Remove-Item $lock -Force
        Write-Host "Lock removido."
    }
}
