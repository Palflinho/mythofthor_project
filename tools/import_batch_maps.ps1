param(
    [string[]]$MapNames = @()
)

$lock = "E:\Dev\mythofthor\coord\build.lock"
$ueCmd = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$uproject = "E:\Dev\mythofthor\mythofthor.uproject"
$batchScript = "E:/Dev/mythofthor/tools/ue_import_batch.py"

if ($MapNames.Count -eq 0) {
    Write-Warning "Nenhum mapa especificado!"
    exit 1
}

$mapsArg = "--maps=" + ($MapNames -join ",")
$scriptArg = "-script=`"$batchScript $mapsArg`""

try {
    "antigravity $(Get-Date -Format s)" | Set-Content $lock -Encoding UTF8
    Write-Host "Lock criado em $lock"
    Write-Host "=========================================="
    Write-Host "Importando mapas em lote: $($MapNames -join ', ')"
    Write-Host "=========================================="
    
    $proc = Start-Process -FilePath $ueCmd -ArgumentList @($uproject, '-run=pythonscript', $scriptArg, '-unattended', '-nopause') -Wait -PassThru -NoNewWindow
    Write-Host "Processo concluido com exit code $($proc.ExitCode)"
} finally {
    if (Test-Path $lock) {
        Remove-Item $lock -Force
        Write-Host "Lock removido."
    }
}
