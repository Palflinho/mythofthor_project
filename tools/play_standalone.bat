@echo off
rem Myth of Thor - roda o jogo fora do editor (Standalone): F1-F9, Alt+atalhos e Enter funcionam
rem como no cliente original (no Play do editor essas teclas sao da Unreal).
rem Usa o caminho sem acento (E:\Dev\mythofthor) por causa do compilador.
set UE="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set PROJ="E:\Dev\mythofthor\mythofthor.uproject"
start "" %UE% %PROJ% /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1600 -ResY=900 -log
