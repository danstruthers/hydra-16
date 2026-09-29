@REM Build the sample programs: each .s is a Hydra executable, bin\NAME.hyx (ld65 with hyx.cfg puts the
@REM header on it).  cc65's ca65 and ld65 must be on the PATH.
@IF NOT EXIST bin MKDIR bin
@IF NOT EXIST obj MKDIR obj
ca65 -g --cpu 65C02 -o obj\hello.o hello.s
@IF ERRORLEVEL 1 EXIT /B 1
ld65 -C hyx.cfg -o bin\hello.hyx obj\hello.o
@IF ERRORLEVEL 1 EXIT /B 1
