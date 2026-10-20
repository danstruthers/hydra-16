/*
** bankprog.h - bank programs: a child program (a subroutine, not a task) kept in a RAM bank of the calling task's own
** and called like any function, on the caller's own C stack (the user's design, 2026-10-18: Dingus Quest Zero's game
** plan, Phase 3B).
**
** A child is built from its folder (bank/NAME: its .c and .s files, linked with sdk/c/child.cfg) into NAME.bp, an o65
** module linked at $8000; the build lists what it uses of its callers (cc65's runtime's helpers, their functions, their
** data) and fails if a caller it names (bank/NAME/parents) doesn't have them.  A caller's exports by name are its
** NAME.exp, written by the build beside its NAME.hyx.  bank_fn loads a child the first time (into a bank the task
** allocates, its imports patched from the caller's exports, its own zero page put in the bytes the caller gives its
** children), and gives a handle to one of its entries; bank_call calls it: the bank mapped at $8000, the call, the
** caller's bank put back (the hub's shared bank too: $00 alone is changed).  A child stays loaded, a call costs some
** 60 cycles more than a JSR; when a bank is wanted and the task has none to spare (BANK_MAX children, or the kernel's
** banks all taken), the child called least lately is given up, and loaded again (from its file) if it's called again.
**
** What a child can't do: it has the $8000 window while it runs, so it can't map another bank there (the hub's), and it
** can call only the caller's main RAM's code (not its overlays, nor its swapped parts); its code, data and BSS fit one
** bank (8K, less its exports' table).  It works on the caller's RAM: its arguments and its results are the caller's
** pointers, read as soon as it returns.
**
**   bank_setup ("/lib/me.exp", 0x70, 16);       (once: the caller's exports, and its children's zero page)
**   h = bank_fn ("/lib/gen.bp", "make");        (the child's _make: NULL if it can't be loaded, bank_err why)
**   n = bank_call (h, int __fastcall__ (*)(int), 3);
*/

#ifndef _BANKPROG_H
#define _BANKPROG_H

#define BANK_MAX        6                       /* Children kept loaded at once */
#define BANK_FNS        12                      /* Entries asked for (bank_fn's handles) */

/* Why a child couldn't be loaded, or an entry found (bank_err) */
enum {
    BE_NONE,                                    /* (None) */
    BE_SETUP,                                   /* bank_setup not called, or no room for another entry */
    BE_FILE,                                    /* Its file isn't there, or not read whole */
    BE_FORMAT,                                  /* Not a child: not o65, not linked at $8000, not one bank */
    BE_ZP,                                      /* Its zero page more than the caller gives */
    BE_EXPORTS,                                 /* The caller's exports' file isn't there */
    BE_IMPORT,                                  /* It uses what the caller doesn't have (bank_missing: its name) */
    BE_BANK,                                    /* No bank to load it into */
    BE_ENTRY                                    /* No such entry */
};
extern unsigned char bank_err;
extern char bank_missing[16];

/* An entry's handle (bank_fn's: the loader's own record, good till the program ends; a call through it loads the
** child again if it was given up) */
struct bank_fn {
    const char*     child;                      /* (Its file, and its entry's name: the caller's strings, kept) */
    const char*     entry;
    unsigned char   slot, gen;                  /* (The child's place in the loader's table, and its load) */
    void*           at;                         /* (The entry's address in its bank) */
};

/* The caller's exports (its NAME.exp: the path kept, the file read as a child's loaded) and the zero page it gives
** its children (n bytes from zp: none the caller's own, nor cc65's runtime's).  0, or -1 */
int __fastcall__ bank_setup (const char* exports, unsigned char zp, unsigned char n);

/* Child's entry by its C name (child, entry: kept, the caller's), the child loaded if it isn't: a handle, or NULL
** (bank_err) */
struct bank_fn* __fastcall__ bank_fn_ (const char* child, const char* entry);
#define bank_fn(child, entry)   bank_fn_ ((child), "_" entry)   /* (Its name as the linker has it: "_" and the C name) */

/* A call through handle h: bank_call (h, the function's type, its arguments).  A handle whose child can't be loaded
** again gives 0xFFFF (bank_err why) */
extern struct bank_fn* bank_go;
void bank_jsr (void);
#define bank_call(h, type, ...) (bank_go = (h), ((type) bank_jsr) (__VA_ARGS__))

/* Every child given up (their banks freed) */
void bank_flush (void);

#endif
