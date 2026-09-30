@REM Build the C library, lib\hydra.lib (cc65's none.lib, with the Hydra's start-up, calls, files, environment,
@REM conio, sound and the rest added or put in place of cc65's: lib\crt, io, env, conio, snd, sys), and the samples
@REM (samples\*.c -> bin\NAME.hyx).  cc65's snapshot is in CC65_HOME (default C:\source\cc65\win64_snapshot).
@REM Then hyc.bat builds a program of your own.
@SETLOCAL
@IF NOT DEFINED CC65_HOME SET CC65_HOME=C:\source\cc65\win64_snapshot
@SET PATH=%CC65_HOME%\bin;%PATH%
@CD /D %~dp0
@IF NOT EXIST obj MKDIR obj
@IF NOT EXIST bin MKDIR bin
@SET OBJS=
@FOR %%D IN (crt io env conio snd sys) DO @FOR %%F IN (lib\%%D\*.s) DO @CALL :asm %%F || EXIT /B 1
@FOR %%D IN (crt io env conio snd sys) DO @FOR %%F IN (lib\%%D\*.c) DO @CALL :cc %%F || EXIT /B 1
COPY /Y /B %CC65_HOME%\lib\none.lib lib\hydra.lib >NUL
ar65 a lib\hydra.lib %OBJS%
@IF ERRORLEVEL 1 EXIT /B 1
@FOR %%F IN (samples\*.c) DO @CALL "%~dp0hyc.bat" "%~dp0%%F" || EXIT /B 1
@EXIT /B 0

@REM (A module's name is its file's: one named as one of cc65's, e.g. getenv, takes its place in the library)
:asm
ca65 -g --cpu 65C02 -I lib -I %CC65_HOME%\asminc -o obj\%~n1.o %1
@IF ERRORLEVEL 1 EXIT /B 1
@SET OBJS=%OBJS% obj\%~n1.o
@EXIT /B 0

:cc
cc65 -g -t none --cpu 65C02 -O -I include -I %CC65_HOME%\include -o obj\%~n1.s %1
@IF ERRORLEVEL 1 EXIT /B 1
ca65 -g --cpu 65C02 -I %CC65_HOME%\asminc -o obj\%~n1.o obj\%~n1.s
@IF ERRORLEVEL 1 EXIT /B 1
@SET OBJS=%OBJS% obj\%~n1.o
@EXIT /B 0
