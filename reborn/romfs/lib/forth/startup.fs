\ startup.fs - what HyForth loads as it starts: forth runs it first, before a script or its prompt.  Through the
\ /lib union, a /lib/forth/startup.fs on the RAM disk or a card takes the place of this one (the ROM's).  The core is
\ Core's words; each library is another word set, pre-compiled (REQUIRE NAME.fl), loaded into the dictionary.

REQUIRE coreext.fl     \ Core Extension
REQUIRE exception.fl   \ Exception: CATCH THROW
REQUIRE file.fl        \ File Access
REQUIRE tools.fl       \ Programming-Tools: .S ? WORDS DUMP SEE [IF] ...

\ And, when they're wanted (here, or in a program):
\ REQUIRE facility.fl   \ Facility: KEY? MS TIME&DATE AT-XY PAGE, structures
\ REQUIRE string.fl     \ String: COMPARE SEARCH SUBSTITUTE ...
\ REQUIRE search.fl     \ Search-Order: word lists, LIBRARY
\ REQUIRE double.fl     \ Double-Number (2CONSTANT 2VARIABLE 2LITERAL, DNEGATE DABS)
\ REQUIRE hydra.fl      \ The Hydra's: sys- words, SH RUN, banks, ARGC ARG
\ REQUIRE hydra.fs      \ The system's constants and error codes (a source library)
