@REM Build the OS ROM: the images go in bin\ (in source control), everything else in obj\ (not)
@IF NOT EXIST bin MKDIR bin
@IF NOT EXIST obj MKDIR obj
ca65 -g -o obj\all_C02.o -l obj\all_C02.txt --cpu 65C02 all.s
ld65 -C os_rom_C02.cfg obj\all_C02.o -Ln obj\os_rom_C02.lbl -m obj\os_rom_C02.map --dbgfile obj\os_rom_C02.dbg
@REM Check for calls to another ROM page that don't go through a gate (needs Node.js)
node tools\check_pages.js obj\os_rom_C02.dbg
