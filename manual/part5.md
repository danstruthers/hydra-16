# Part Five. Machine Language

Underneath every program on the Hydra, HyForth, hylang and HyBASIC included, the processor is doing one simple
thing over and over: reading an instruction from memory and doing it. This part goes down to that level. It explains
the 65C02 processor, then the three tools for programming it directly: the **assembler**, which turns your
instructions into machine code; the **disassembler**, which turns machine code back into instructions you can read;
and the **debugger**, which runs a program one instruction at a time while you watch. Appendix D lists every
instruction.

* Chapter 26. The 65C02
* Chapter 27. The Assembler
* Chapter 28. The Disassembler
* Chapter 29. The Debugger
