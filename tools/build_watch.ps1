# Build do MythOfThorEditor com vigia: se 5 min sem progresso (CPU do cl/dotnet parada e log sem crescer),
# mata o build e sai com "TRAVOU". Uso: powershell -File build_watch.ps1 [-StallMinutes 5]
param([int]$StallMinutes = 5, [string]$Quem = 'claude')
$lock = Join-Path $PSScriptRoot '..\coord\build.lock'
$log = "$env:TEMP\build.log"
$bat = "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
$args = @('MythOfThorEditor', 'Win64', 'Development', '-Project=E:\Dev\mythofthor\mythofthor.uproject', '-WaitMutex', '-NoUBA', '-NoUBALocal')
# Outro build rodando (ex.: Antigravity)? Nao comeca por cima
$other = Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" | Where-Object { $_.CommandLine -match 'UnrealBuildTool' }
if ($other) { "OUTRO BUILD RODANDO (pid $($other.ProcessId -join ',')): nao iniciei"; exit 3 }
if (Test-Path $lock) { "BUILD TRAVADO PELO LOCK: $(Get-Content $lock -Raw)"; exit 3 }
"$Quem $(Get-Date -Format s)" | Set-Content $lock -Encoding UTF8
Get-Process UnrealEditor -ErrorAction SilentlyContinue | Stop-Process -Force -PassThru | Wait-Process -Timeout 30 -ErrorAction SilentlyContinue  # a DLL fica presa ate o processo sair
$p = Start-Process -FilePath $bat -ArgumentList $args -RedirectStandardOutput $log -RedirectStandardError "$log.err" -PassThru -WindowStyle Hidden
$lastCpu = -1.0; $lastLen = -1; $stallSince = Get-Date; $start = Get-Date
while (-not $p.HasExited) {
    Start-Sleep -Seconds 30
    # Tempo de CPU pelo WMI (Get-Process .CPU volta vazio para processos do executor do UBA): em segundos
    $cpu = (Get-CimInstance Win32_Process -Filter "Name='cl.exe' OR Name='link.exe' OR Name='dotnet.exe' OR Name='UnrealHeaderTool.exe'" |
        ForEach-Object { ($_.KernelModeTime + $_.UserModeTime) / 1e7 } | Measure-Object -Sum).Sum
    if ($null -eq $cpu) { $cpu = 0 }
    $len = (Get-Item $log -ErrorAction SilentlyContinue).Length
    if ([math]::Abs($cpu - $lastCpu) -gt 2 -or $len -ne $lastLen) { $stallSince = Get-Date }
    $lastCpu = $cpu; $lastLen = $len
    if (((Get-Date) - $stallSince).TotalMinutes -ge $StallMinutes) {
        Get-Process cl, link, UbaHost -ErrorAction SilentlyContinue | Stop-Process -Force
        Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'" | Where-Object { $_.CommandLine -match 'UnrealBuildTool' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        "TRAVOU: $StallMinutes min sem progresso (total {0:N0} min). Ultimas linhas:" -f ((Get-Date) - $start).TotalMinutes
        Get-Content $log -Tail 6
        Remove-Item $lock -ErrorAction SilentlyContinue
        exit 2
    }
}
Remove-Item $lock -ErrorAction SilentlyContinue
"Terminou em {0:N1} min" -f ((Get-Date) - $start).TotalMinutes
Select-String -Path $log -Pattern 'error C|error LNK|Result:' | Select-Object -First 15 | ForEach-Object Line
