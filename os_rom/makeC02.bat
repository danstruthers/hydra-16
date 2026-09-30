@REM Build the OS ROM: the images go in bin\ (in source control), everything else in obj\ (not)
@IF NOT EXIST bin MKDIR bin
@IF NOT EXIST obj MKDIR obj
@REM (A failed step stops the build: otherwise the link would use the last good object file)
@REM The test song (sndtest's, paged ROM bank 2), from its score (needs Node.js: sim/tools/hysong.js)
node ..\sim\tools\hysong.js songs\test.mml songs\test.zsm --rom songs\test_rom.s --quiet
@IF ERRORLEVEL 1 EXIT /B 1
ca65 -g -o obj\all_C02.o -l obj\all_C02.txt --cpu 65C02 all.s
@IF ERRORLEVEL 1 EXIT /B 1
ld65 -C os_rom_C02.cfg obj\all_C02.o -Ln obj\os_rom_C02.lbl -m obj\os_rom_C02.map --dbgfile obj\os_rom_C02.dbg
@IF ERRORLEVEL 1 EXIT /B 1
@REM The ROMs' checksums, for the hardware test (in the paged ROM image)
node tools\romsum.js bin\os_rom_C02.bin bin\paged_rom_C02.bin
@IF ERRORLEVEL 1 EXIT /B 1
@REM Check for calls to another ROM page that don't go through a gate (needs Node.js)
node tools\check_pages.js obj\os_rom_C02.dbg
@REM makeC02 test: then run the regression tests in the emulator (sim\regress.js)
@IF /I "%1"=="test" node ..\sim\regress.js
