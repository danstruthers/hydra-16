# Appendix J. Glossary

The words this book uses, and what they mean. The number in brackets after a word is the section where it's
explained in full.

**Accumulator.** The 65C02's A register, where its arithmetic and logic are done (24.3).

**ACIA.** Asynchronous Communications Interface Adapter: the 65C51 chip, U3, that runs the serial port (6.2).

**Address.** The number of a byte in memory. The 65C02's addresses go from $0000 to $FFFF (6.2).

**Addressing mode.** The way an instruction says which byte it works on: `#$41`, `$22,x`, `($22),y` ... (24.6).

**Argument.** A word given to a command after its name: in `ls -l /rom`, `-l` and `/rom` are arguments (7.2).

**ASCII.** The standard code for characters, from 0 to 127: `A` is 65 (Appendix E).

**Assembler.** A program that turns assembly language into machine code. The Hydra's is `as` (Chapter 25).

**Assembly language.** A program written with the processor's instruction names, like `LDA`, instead of numbers
(24.1).

**Bank.** A piece of a bigger memory that can be shown through a window in the 65C02's 64K: an 8K RAM bank at
$8000, a 16K ROM bank at $A000 (6.2).

**Base.** How many digits a way of writing numbers uses: 10 for decimal, 2 for binary, 16 for hex (17.3).

**Base string.** The text that names a base in HydraOS's languages: `x` for hex, `b` for binary, `7r` for base 7
(17.3).

**Binary.** Numbers written in base 2, with only the digits 0 and 1 (1.6).

**Bind.** To make a file or directory appear at another name in the namespace (8.5).

**Bit.** A binary digit, 0 or 1: the smallest piece of information (1.6).

**Bitmap.** A picture made of dots, each dot's colour kept in memory (15.4).

**Block.** A piece of a disk: 512 bytes on HydraFS (13.5). In HyForth, a numbered 1K piece of a file of blocks
(19.10).

**Boot.** Starting the computer: everything from the reset to the first prompt (Chapter 6).

**Breakpoint.** A place in a program where the debugger stops it (27.5).

**BSS.** The part of a program for variables that start as zero. It isn't stored in the program's file (25.8).

**Bug.** A mistake in a program. Finding bugs is **debugging** (Chapter 27).

**Bus.** The wires that carry addresses, data and control signals between the processor and the other chips (1.2).

**Byte.** Eight bits: a number from 0 to 255 (1.6).

**Call registers.** r0 to r15: sixteen 2-byte places on the zero page, for passing values to and from system calls
(6.3).

**Capacitor.** A part that stores a little electric charge. Most of the Hydra's smooth out the power beside each chip
(3.4).

**Card.** A board that plugs into one of the Hydra's slots, like the Vera X; or an SD card.

**Carry.** The C flag: the bit carried out of an addition, or shifted out of a byte (24.8).

**Cell.** In HyForth, one item on the stack: 16 bits (18.2).

**Channel.** One voice of the sound chips. The Hydra has 24: 8 on the YM2151 and 16 on the Vera X (Chapter 14).

**Chip.** An integrated circuit: a tiny circuit in a plastic package with metal legs (3.4).

**Clock.** The signal whose ticks pace the processor: 3.58 million a second (6.1). Also the clock chip, which keeps
the date and time (16.5).

**Command.** A line typed for the shell to do; or a program it runs (7.2).

**Comment.** Text in a program for people to read, which the computer skips.

**Console.** The keyboard and screen you use the Hydra through: the serial terminal, or the Vera X's screen
(Chapter 12).

**Constant.** A name for a value that doesn't change.

**Control character.** A code from 0 to 31 that tells a terminal to do something, instead of printing (Appendix E).

**Control file.** A device's file, usually called `ctl`, that takes commands written to it as text (11.4).

**Cooked mode.** The console's usual mode, in which a program gets a whole line at a time (12.2).

**Current directory.** The directory a program is "in": a file name that doesn't start with `/` is found there
(8.1).

**Cycle.** One tick of the processor's clock (24.12).

**Debugger.** A program that runs another one step by step, to find its bugs: `db` (Chapter 27).

**Decimal.** Base 10, the usual way of writing numbers.

**Device.** A part of the Hydra that does input or output: the serial port, the sound chips, the SD card (Chapter
11).

**Dictionary.** In HyForth, the list of all the words it knows (18.5).

**Directive.** An instruction to the assembler itself, starting with a dot: `.byte` (25.4).

**Directory.** A file that holds other files: a folder (8.1).

**Disassembler.** A program that turns machine code back into assembly language: `dis` (Chapter 26).

**Driver.** A program that looks after a device, and gives other programs files to use it with (Chapter 11).

**Emulator.** A program that imitates a computer. The Hydra emulator runs HydraOS on a PC (Chapter 2).

**Environment.** A task's variables, each a name and a value, which programs can read: `$path`, `$window` (7.4).

**Error code.** A number that says what went wrong in a system call (Appendix G).

**Escape sequence.** A few characters starting with ESC that tell a terminal to move the cursor, change colour, and
so on (12.7).

**Expression.** A calculation, like `ROWS + 1`.

**File.** A named piece of data on a disk. In HydraOS, devices are files too (Chapter 8).

**File descriptor (fd).** A small number that stands for a file a program has open (25.13).

**Flag.** A bit that says yes or no, like the 65C02's N, V, D, I, Z and C flags (24.3). In Forth, a true or false
value (18.7). And an option, like `-l`.

**Flash ROM.** ROM that can be erased and programmed again electrically (4.4).

**FM synthesis.** Making sounds by changing one wave's frequency with another wave: the YM2151's way (14.1).

**Font.** The pictures of the characters, 8 dots by 8 on the Vera X (Appendix E).

**Function.** A named piece of a program that can be given values (its arguments) and gives one back.

**Gateware.** The configuration that makes an FPGA chip, like the Vera X's, into a particular circuit.

**Header.** A row of metal pins on a board (3.4). Also the first 48 bytes of a program file, which describe it (25.8).

**Hexadecimal (hex).** Base 16: the digits 0 to 9 and A to F. $FF is 255 (1.6).

**HydraFS.** The Hydra's file system: the way it keeps files on its disks (13.5).

**HydraOS.** The Hydra's operating system (1.5).

**I/O.** Input and output (1.2).

**I²C.** A bus of two wires for talking to small chips, on the I/O header (16.3).

**Index register.** X or Y: registers for counting, and for stepping through tables (24.3).

**Instruction.** One step the processor can do (24.1).

**Integer.** A whole number (17.1).

**Interrupt.** A signal from a chip that makes the processor stop what it's doing to deal with it (24.11).

**Jumper.** A small cap that connects two pins of a header (4.6).

**Kernel.** The heart of HydraOS, which runs the tasks, the files and the namespaces (6.1).

**Label.** A name for a place in an assembly language program (25.5).

**Library.** A collection of words, functions or routines that programs can use.

**Little-endian.** Keeping a 16-bit number with its low byte first, as the 65C02 does (24.2).

**Login shell.** The shell each window starts with (7.16).

**Machine code.** A program as the processor reads it: bytes (24.1).

**Macro.** A named piece of source that is put in wherever its name is used (25.11).

**Memory card.** The board that holds the Hydra's extra RAM, plugged into J1 and J2 (4.7).

**Mnemonic.** An instruction's short name, like `LDA` (24.1).

**Module.** A program or driver kept in the paged ROM (6.4). Also a **RAM module**: 2 MB of RAM on a memory card
(4.7).

**Monitor.** The emulator's own command line, for looking inside the processor (2.4).

**Mount.** To attach a device's files to the namespace (8.5).

**Namespace.** The tree of names a task sees, made by binds and mounts (8.5).

**Note.** A message to a task, like `interrupt` (<kbd>CTRL</kbd>-<kbd>C</kbd>). A **note group** is the tasks that
get a note together (25.16).

**Opcode.** The number of an instruction (24.1).

**Operand.** What an instruction works on: a number, or an address (24.1).

**Operating system.** The program that runs a computer, and the other programs on it (1.5).

**Option.** A word starting with `-` that changes what a command does: the `-l` in `ls -l` (7.2).

**Page.** 256 bytes of memory, from $xx00 to $xxFF (24.5). Also one 8K page of the kernel's ROM (6.2).

**Paged ROM.** The Hydra's ROM of up to 4 MB, seen 16K at a time at $A000 (6.2).

**Palette.** The colours a screen can show (15.5).

**Parameter.** A name for a value that a function or macro is given.

**Partition.** A part of a disk that's used as if it were a disk of its own (13.2).

**Patch.** The settings that make the sound chip sound like an instrument (14.4).

**Path.** A file's name with its directories: `/sd/0/sample/hi` (8.1).

**Pipe.** A connection that carries one program's output into another's input: `|` (7.7).

**Pointer.** An address kept in memory, which points at something else (24.6).

**Port.** A group of 16 addresses for a chip (6.2); or a connector.

**Processor.** The chip that runs programs: the W65C02S (Chapter 24).

**Prompt.** What a program prints when it's waiting for you to type, like `/>`.

**PSG.** The Vera X's programmable sound generator, with 16 simple voices (14.1).

**RAM.** Random-access memory: memory that can be read and written, and forgets everything when the power goes off.

**RAM disk.** A disk made of RAM: `/ram` (13.3).

**Raw mode.** The console's mode in which a program gets each key as it's pressed (12.3).

**Recursion.** A function (or word) that calls itself (18.14).

**Register.** A small memory inside the processor (A, X, Y ...: 24.3), or a chip's control location, or one of the
Hydra's own T, U, V and W (6.2).

**Reset.** Starting the processor again from the beginning: the RESET button (6.1).

**Resistor.** A part that limits the flow of electric current (3.4).

**ROM.** Read-only memory, which keeps what's in it without power.

**ROM disk.** The files kept in the paged ROM: `/rom` (13.3).

**Scheduler.** The part of the kernel that decides which task runs next (6.3).

**Script.** A file of commands for a shell or a language to run (7.13).

**SD card.** The small memory card the Hydra uses as a disk (13.2).

**Segment.** A part of an assembly language program: `CODE`, `DATA`, `BSS` ... (25.8). Also a shared memory segment
(16.9).

**Serial port.** The connector J3, which sends and receives data one bit at a time: the Hydra's console (12.6).

**Server.** A driver with files of its own (11.2).

**Shell.** The program that reads your commands and runs them: HyForth, rc, hylang or HyBASIC (Chapter 7).

**Slot.** One of the six connectors for expansion cards, J12 to J17 (1.4).

**Socket.** A holder soldered to the board, which a chip plugs into (3.4).

**Solder.** A metal that melts at a low temperature, used to join parts to the board (3.5).

**Source.** A program as it's written, as text.

**SPI.** A simple serial bus for chips and cards. The Hydra has eight SPI device headers (16.4).

**Sprite.** A small picture that can move over the screen without changing what's under it (15.5).

**Stack.** A pile of values where the last one put on is the first taken off. The 65C02 has one (24.10), and so
does Forth (18.2).

**Standard input, output and error.** A program's fds 0, 1 and 2: usually the keyboard and the screen (25.13).

**Status.** What a program said when it ended: nothing for success, or a message (7.9).

**String.** Text, as a value in a program.

**Subroutine.** A piece of code that's called with `JSR` and returns with `RTS` (24.10).

**System call.** A request from a program to HydraOS: open a file, start a program, wait (25.13).

**Task.** One of the sixteen programs the Hydra can run at once, each with its own 32K of memory (6.3).

**Terminal.** A screen and keyboard for a computer; or a program that acts as one (5.2).

**Tick.** One of HydraOS's timer interrupts: 200 a second (6.3).

**Two's complement.** The way a byte holds a negative number: $FF is -1 (24.2).

**Union directory.** A directory made of several, which are looked in in turn (8.7).

**Variable.** A name for a value that can change.

**VERA.** The video chip on the Vera X card (15.1).

**VIA.** Versatile Interface Adapter: the 65C22 chip, U2, with the Hydra's timer and its I/O pins (6.2).

**Window.** A part of the 65C02's 64K through which a bigger memory is seen (6.2). Also one of the console's text
windows (Chapter 9).

**Word.** In Forth, a named command (18.1). Also a 16-bit value.

**Zero page.** The first 256 bytes of memory, $0000 to $00FF, which the 65C02 reaches with shorter, quicker
instructions (24.5).
