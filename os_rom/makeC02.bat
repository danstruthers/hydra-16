DEL .\tmp\*.o
REM ca65 -o tmp\all_C02.o -l tmp\all_C02.txt -I C:\source\cc65\asminc --cpu 65C02 all.s 
ca65 -o tmp\all_C02.o -l tmp\all_C02.txt --cpu 65C02 all.s 
ld65 -C os_rom_C02.cfg tmp\all_C02.o -o tmp\os_rom_C02.bin -Ln tmp\os_rom_C02.lbl