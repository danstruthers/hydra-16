\ startup.fs - what HyForth loads as it starts: forth runs it first, before a script or its prompt.  Through the
\ /lib union, a /lib/forth/startup.fs on the RAM disk or a card takes the place of this one (the ROM's).  The core is
\ Core's words; each library is another word set, pre-compiled (require NAME.fl), loaded into the dictionary.

require coreext.fl     \ Core Extension
require exception.fl   \ Exception: catch throw
require file.fl        \ File Access
require tools.fl       \ Programming-Tools: .s ? words dump see [if] ...

\ And, when they're wanted (here, or in a program; or at the prompt, lib NAME):
\ require facility.fl   \ Facility: key? ms time&date at-xy page, structures
\ require string.fl     \ String: compare search substitute ...
\ require search.fl     \ Search-Order: word lists, library
\ require double.fl     \ Double-Number (2constant 2variable 2literal, dnegate dabs)
\ require hydra.fl      \ The Hydra's: sys- words, sh run, banks, argc arg, sys
\ require hydra.fs      \ The system's constants and error codes (a source library)
\ require disasm.fl     \ The disassembler: disasm, and see of a code word
\ require bits.fl       \ tbit sbit cbit
\ require random.fl     \ rand rand32 rseed random
\ require numbers.fl    \ hylang's numbers on a number stack: n+ n. nsqrt set-base ...
