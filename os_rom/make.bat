ca65 os_main.s -o tmp\os_main.o -l tmp\os_main.txt -I C:\source\cc65\asminc --cpu 6502
ld65 -C os_rom.cfg tmp\os_main.o -o tmp\os_main.bin -Ln tmp\os_main.lbl