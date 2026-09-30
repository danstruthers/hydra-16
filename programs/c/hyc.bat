@REM hyc.bat FILE.c [MORE.c | MORE.s ...]: a C program for the Hydra, bin\FILE.hyx (and bin\FILE.map), with the C
@REM library (lib\hydra.lib: make.bat builds it) and hydra.cfg.  .s files are assembly (ca65; lib\hydra.inc has the
@REM OS's calls).  HYC_CFLAGS and HYC_LDFLAGS, if set, go to cc65 and ld65 (e.g. --check-stack, -D __STACKSIZE__=$400).
@REM Put it on a card and run it by name, as any program.
@SETLOCAL
@IF NOT DEFINED CC65_HOME SET CC65_HOME=C:\source\cc65\win64_snapshot
@SET PATH=%CC65_HOME%\bin;%PATH%
@SET HERE=%~dp0
@IF "%~1"=="" ECHO hyc.bat FILE.c [MORE.c ^| MORE.s ...] & EXIT /B 1
@IF NOT EXIST %HERE%obj MKDIR %HERE%obj
@IF NOT EXIST %HERE%bin MKDIR %HERE%bin
@SET NAME=%~n1
@SET OBJS=
:next
@IF "%~1"=="" GOTO link
@IF /I "%~x1"==".s" GOTO asm
cc65 -g -t none --cpu 65C02 -O %HYC_CFLAGS% -I %HERE%include -I %CC65_HOME%\include -o %HERE%obj\%~n1.s %1
@IF ERRORLEVEL 1 EXIT /B 1
ca65 -g --cpu 65C02 -I %CC65_HOME%\asminc -o %HERE%obj\%~n1.o %HERE%obj\%~n1.s
@IF ERRORLEVEL 1 EXIT /B 1
@GOTO added
:asm
ca65 -g --cpu 65C02 -I %HERE%lib -I %CC65_HOME%\asminc -o %HERE%obj\%~n1.o %1
@IF ERRORLEVEL 1 EXIT /B 1
:added
@SET OBJS=%OBJS% %HERE%obj\%~n1.o
@SHIFT
@GOTO next
:link
ld65 -C %HERE%hydra.cfg %HYC_LDFLAGS% -m %HERE%bin\%NAME%.map -o %HERE%bin\%NAME%.hyx %OBJS% %HERE%lib\hydra.lib
@IF ERRORLEVEL 1 EXIT /B 1
@EXIT /B 0
