ca65 os_main.s -o tmp\os_main_C02.o -l tmp\os_main_C02.txt -I C:\source\cc65\asminc --cpu 65C02
ld65 -C os_rom_C02.cfg tmp\os_main_C02.o -o tmp\os_main_C02.bin -Ln tmp\os_main_C02.lbl