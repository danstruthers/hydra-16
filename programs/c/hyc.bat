@REM hyc.bat FILE.c [MORE.c ...]: a C program for the Hydra, bin\FILE.hyx (and bin\FILE.map), with the C library
@REM (lib\hydra.lib: make.bat builds it) and hydra.cfg.  Put it on a card and run it by name, as any program.
@SETLOCAL
@IF NOT DEFINED CC65_HOME SET CC65_HOME=C:\source\cc65\win64_snapshot
@SET PATH=%CC65_HOME%\bin;%PATH%
@SET HERE=%~dp0
@IF "%~1"=="" ECHO hyc.bat FILE.c [MORE.c ...] & EXIT /B 1
@IF NOT EXIST %HERE%obj MKDIR %HERE%obj
@IF NOT EXIST %HERE%bin MKDIR %HERE%bin
@SET NAME=%~n1
@SET OBJS=
:next
@IF "%~1"=="" GOTO link
cc65 -g -t none --cpu 65C02 -O -I %HERE%include -I %CC65_HOME%\include -o %HERE%obj\%~n1.s %1
@IF ERRORLEVEL 1 EXIT /B 1
ca65 -g --cpu 65C02 -I %CC65_HOME%\asminc -o %HERE%obj\%~n1.o %HERE%obj\%~n1.s
@IF ERRORLEVEL 1 EXIT /B 1
@SET OBJS=%OBJS% %HERE%obj\%~n1.o
@SHIFT
@GOTO next
:link
ld65 -C %HERE%hydra.cfg -m %HERE%bin\%NAME%.map -o %HERE%bin\%NAME%.hyx %OBJS% %HERE%lib\hydra.lib
@IF ERRORLEVEL 1 EXIT /B 1
@EXIT /B 0
