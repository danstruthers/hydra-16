@REM hyc.bat FILE.c [MORE.c | MORE.s ...]: a C program for the Hydra, bin\FILE.hyx (and bin\FILE.map), with the C
@REM library (make.bat builds it).  HYC_CFLAGS and HYC_LDFLAGS, if set, go to cc65 and ld65.  As build.js prog does
@node "%~dp0..\..\build.js" prog %*
