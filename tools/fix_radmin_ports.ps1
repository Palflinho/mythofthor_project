# Run as ADMINISTRATOR (PowerShell). Re-points the Windows port forwarding after the Docker ports moved to 16900/16121/15121
# (the forwarding on 6900/6121/5121 used to collide with Docker inside WSL, which made the server restart forever).
#   Radmin IP :6900/6121/5121  -> 127.0.0.1:16900/16121/15121   (friends)
#   127.0.0.1 :6900/6121/5121  -> 127.0.0.1:16900/16121/15121   (the owner's own game, JogarLocal.bat)
param([string]$RadminIp = '26.184.211.1')
$map = @{ 6900 = 16900; 6121 = 16121; 5121 = 15121 }
foreach ($p in $map.Keys) {
    netsh interface portproxy delete v4tov4 listenaddress=$RadminIp listenport=$p 2>$null | Out-Null
    netsh interface portproxy delete v4tov4 listenaddress=127.0.0.1 listenport=$p 2>$null | Out-Null
    netsh interface portproxy add v4tov4 listenaddress=$RadminIp listenport=$p connectaddress=127.0.0.1 connectport=$($map[$p])
    netsh interface portproxy add v4tov4 listenaddress=127.0.0.1 listenport=$p connectaddress=127.0.0.1 connectport=$($map[$p])
}
Restart-Service iphlpsvc -Force
netsh interface portproxy show v4tov4
