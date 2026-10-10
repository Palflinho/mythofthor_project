# Espera uma mensagem "status: pendente" em coord/para_<agente>/ e imprime o conteudo.
# Uso: powershell -ExecutionPolicy Bypass -File coord/esperar.ps1 -Para antigravity [-IntervaloSeg 20] [-MaxMin 25]
param(
    [Parameter(Mandatory = $true)][ValidateSet('antigravity', 'claude')][string]$Para,
    [int]$IntervaloSeg = 20,
    [int]$MaxMin = 25
)
$dir = Join-Path $PSScriptRoot "para_$Para"
New-Item -ItemType Directory -Force $dir | Out-Null
$inicio = Get-Date
while ($true) {
    $pend = Get-ChildItem $dir -Filter *.md -ErrorAction SilentlyContinue | Sort-Object Name |
        Where-Object { (Get-Content $_.FullName -TotalCount 1 -Encoding UTF8) -match '^status:\s*pendente' }
    if ($pend) {
        foreach ($f in $pend) {
            "===== NOVA MENSAGEM: $($f.FullName) ====="
            Get-Content $f.FullName -Encoding UTF8
            ""
        }
        exit 0
    }
    if (((Get-Date) - $inicio).TotalMinutes -ge $MaxMin) { "NADA NOVO em $MaxMin min (rode de novo)"; exit 1 }
    Start-Sleep -Seconds $IntervaloSeg
}
