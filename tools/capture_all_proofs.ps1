# Script de automação para captura de prints comprobatórios do HD-2D
param(
    [string]$OnlyTest = ''
)

$ue = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$proj = 'E:\Dev\mythofthor\mythofthor.uproject'
$map = '/Game/Maps/Prontera/Lvl_Prontera'
$shotDir = 'E:\Dev\mythofthor\Saved\Screenshots\WindowsEditor'

$tests = @(
    @{
        Name = "proof_1_dungeon_lighting"
        Desc = "Dungeon Mode (prt_sewb1): Sol desligado, ambiente frio, tochas e cristais brilhando"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 9 MythSay+@warp+prt_sewb1+120+120, MythDelayed 17 MythShot+0+12+proof_1_dungeon_lighting, MythDelayed 23 quit"
    },
    @{
        Name = "proof_2_peco_cart_shadow"
        Desc = "Sombra de Peco Peco e Carrinho: Sombra projetada no chao sob montaria e carrinho"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 8 MythSay+@warp+prontera+156+180, MythDelayed 11 MythSay+@job+10, MythDelayed 13 MythSay+@cart+1, MythDelayed 18 MythShot+0+12+proof_2_peco_cart_shadow, MythDelayed 24 quit"
    },
    @{
        Name = "proof_3_loot_beams"
        Desc = "Loot Beams por Raridade: Feixes verticais em itens caidos (Card, Armadura, Yggdrasil)"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 8 MythSay+@warp+prontera+156+180, MythDelayed 11 MythSay+@item+2357+1, MythDelayed 12 MythSay+@item+4001+1, MythDelayed 13 MythSay+@item+607+1, MythDelayed 18 MythShot+0+12+proof_3_loot_beams, MythDelayed 24 quit"
    },
    @{
        Name = "proof_4_cloud_shadows_and_wetness"
        Desc = "Sombras de Nuvens e Wetness: Chuva e piso molhado com reflexo em campo aberto"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 8 MythSay+@warp+prt_fild08+170+170, MythDelayed 12 MythWeather+Rain, MythDelayed 18 MythShot+0+12+proof_4_cloud_shadows_and_wetness, MythDelayed 24 quit"
    },
    @{
        Name = "proof_5_water_ghost_reflection"
        Desc = "Ghost Sprite na Agua: Reflexo do sprite espelhado no eixo Z com onda senoidal"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 8 MythSay+@warp+prt_fild08+172+175, MythDelayed 17 MythShot+0+12+proof_5_water_ghost_reflection, MythDelayed 23 quit"
    },
    @{
        Name = "proof_6_monster_dissolve"
        Desc = "Procedural Death Dissolve: Monstro dissolvendo com borda incandescente"
        Cmds = "MythDelayed 4 MythLogin+mythclaude+mythclaude123, MythDelayed 8 MythSay+@warp+prontera+156+180, MythDelayed 11 MythSay+@monster+1002+1, MythDelayed 14 MythAttackNearest, MythDelayed 16 MythShot+0+-1+proof_6_monster_dissolve, MythDelayed 22 quit"
    }
)

foreach ($t in $tests) {
    if ($OnlyTest -ne '' -and $t.Name -ne $OnlyTest) {
        continue
    }

    Write-Host "==========================================================" -ForegroundColor Cyan
    Write-Host "Executando teste: $($t.Name)" -ForegroundColor Yellow
    Write-Host "Descricao: $($t.Desc)" -ForegroundColor Gray
    Write-Host "==========================================================" -ForegroundColor Cyan

    $argString = "`"$proj`" $map -game -windowed -ResX=1280 -ResY=720 -NoLiveCoding -ExecCmds=`"$($t.Cmds)`""
    $p = Start-Process -FilePath $ue -ArgumentList $argString -PassThru
    Write-Host "Processo iniciado (PID: $($p.Id)). Aguardando..."
    $ok = $p.WaitForExit(180000)
    if (-not $ok) {
        Write-Host "Timeout atingido, matando processo..." -ForegroundColor Red
        $p.Kill()
    } else {
        Write-Host "Processo finalizou com codigo $($p.ExitCode)." -ForegroundColor Green
    }

    $imgPath = Join-Path $shotDir "$($t.Name).png"
    if (Test-Path $imgPath) {
        $size = (Get-Item $imgPath).Length
        Write-Host "Sucesso: Screenshot gerado ($imgPath, $size bytes)" -ForegroundColor Green
    } else {
        Write-Host "Aviso: Imagem $($t.Name).png nao encontrada em $shotDir" -ForegroundColor Red
    }
}
