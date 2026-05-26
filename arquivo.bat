@echo off
title Bomberman - Modo Emojis & AutoPlay
cls
echo =======================================================
echo   Iniciando Bomberman no Windows Terminal Moderno...
echo =======================================================
echo.

:: Verifica se o executável 'main.exe' existe na pasta
if not exist "main.exe" (
    set /p NOME_EXE="[AVISO] Nao encontrei o arquivo 'main.exe'. Qual o nome do seu executavel gerado? (ex: jogo.exe): "
) else (
    set NOME_EXE=main.exe
)

:: Executa o jogo forçando uma nova aba do Windows Terminal configurada em UTF-8 (65001)
wt -d . cmd /k "chcp 65001 > nul && %NOME_EXE%"

exit
