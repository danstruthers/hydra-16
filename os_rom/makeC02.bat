@REM Build the OS ROM: the images go in bin\ (in source control), everything else in obj\ (not)
@IF NOT EXIST bin MKDIR bin
@IF NOT EXIST obj MKDIR obj
@REM (A failed step stops the build: otherwise the link would use the last good object file)
ca65 -g -o obj\all_C02.o -l obj\all_C02.txt --cpu 65C02 all.s
@IF ERRORLEVEL 1 EXIT /B 1
ld65 -C os_rom_C02.cfg obj\all_C02.o -Ln obj\os_rom_C02.lbl -m obj\os_rom_C02.map --dbgfile obj\os_rom_C02.dbg
@IF ERRORLEVEL 1 EXIT /B 1
@REM Check for calls to another ROM page that don't go through a gate (needs Node.js)
node tools\check_pages.js obj\os_rom_C02.dbg
