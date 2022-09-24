@echo off

SET projectroot=%cd:~0,-13%
echo %projectroot%

SUBST x: /D
SUBST x: %projectroot%

pushd x:

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
start D:\Development\Vim\vim80\gvim.exe
