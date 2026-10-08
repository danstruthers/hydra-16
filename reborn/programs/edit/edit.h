/*
** edit.h - the screen editor's parts: edit.c (the keys, the commands, the files open), text.c and blocks.s (a file's
** text, in the task's RAM banks), undo.c (undo, redo and the cut buffer, in banks too) and screen.c (the terminal).
**   A file's text is a row of blocks, a bank each (8K at $8000), each with a gap in it where its text is changed:
** a block's text is its bank's bytes from 0 to its gap's start (gs), then from its gap's end (ge) to 8K.  Typing
** moves a block's gap to the cursor and puts the byte in it; a block whose gap is full is split in two.  So a
** change costs the same anywhere in a big file, and a place in a block is 16 bits.  A place in the whole text is a
** count of its bytes from the start (lpos).  The file shown's blocks, cursor and iterator are blocks.s's (bstate:
** saved and put back whole, with D, as the file shown changes); the others' are kept in a bank (edit.c).
*/

#include <hydra.h>

#define BANK        ((unsigned char*) 0x8000)   /* The bank window: a block's bytes */
#define BLK         0x2000                      /* A block: a bank */
#define BLK_MAX     48                          /* A file's blocks, at most (384K) */
#define LOAD_FILL   0x1800                      /* A block's text as a file is read (6K: room to type in) */
#define DOC_MAX     6                           /* Files open at once */
#define PAT_MAX     64                          /* A pattern (find, replace) */
#define CHUNK       256                         /* Bytes between banks: through bounce */
#define LF          '\n'
#define TAB         8                           /* Tab stops */
#define BSTATE      (1 + BLK_MAX * 7 + 9)       /* blocks.s's state's bytes */

typedef unsigned long lpos;                     /* A place in the text: bytes from its start */
#define NOWHERE     ((lpos) -1)

/* A file open: where the screen is in it, its mark, its undo log, its name */
struct doc {
    unsigned        want;                       /* The column the cursor keeps going up and down */
    lpos            top;                        /* The screen's first line: its place, */
    unsigned        topline;                    /*   its number, */
    unsigned        left;                       /*   and the screen's first column */
    lpos            mark;                       /* The mark: a block's other end (marked) */
    unsigned char   marked;
    unsigned char   changed;                    /* Changed since it was read or written */
    unsigned char   dos;                        /* Its lines ended CR LF (and will again) */
    unsigned char   ubank;                      /* Its undo log: its bank ($FF: none), */
    unsigned        utop;                       /*   the top of what's done, */
    unsigned        uend;                       /*   the end of what's undone (redo's) */
    unsigned char   ugroup;                     /* The last record may grow (U_*): typing, deleting, rubbing out */
    char            name[HY_PATH_MAX + 1];      /* Its file ("": none yet) */
};

extern struct doc D;                            /* The file shown */
extern unsigned char bounce[CHUNK];             /* Bytes on their way between banks */

/* blocks.s: the file shown's blocks (each 16-bit table a low and a high one), its cursor; the iterator.  (The
** cursor's moves use the iterator: it's to be set again after them) */
extern unsigned char bstate[BSTATE];
extern unsigned char nblk, bkb[], gsl[], gsh[], gel[], geh[], nll[], nlh[];
extern unsigned char cb;                        /* The cursor: its block, */
extern unsigned co;                             /*   its offset in the block's text, */
extern lpos cpos;                               /*   its place, */
extern unsigned cline;                          /*   and its line (0: the first) */
unsigned __fastcall__ blen (unsigned char b);   /* Block b's text's length */
int t_get (void);                               /* The byte at the cursor (-1: the end) */
int t_next (void);                              /* The cursor past a byte: it (-1: at the end) */
int t_prev (void);                              /* The cursor back over a byte: it (-1: at the start) */
void __fastcall__ t_goto (lpos p);              /* The cursor to p */
void __fastcall__ it_set (lpos p);              /* The iterator, for looking along the text: to p */
int it_next (void);                             /* Its byte, and it past it (-1: the end) */
int it_prev (void);                             /* Back over a byte: it (-1: the start) */
lpos it_pos (void);                             /* Where it is */
unsigned __fastcall__ lfs (const unsigned char* s, unsigned n);    /* The LFs in n bytes at s */
void gapcur (void);                             /* The cursor's block's gap to the cursor */
unsigned __fastcall__ ins1 (const unsigned char* s, unsigned n);   /* What room there is of n bytes in there */
unsigned __fastcall__ del1 (unsigned n);        /* Up to n bytes (CHUNK) after it out, into bounce */
unsigned char __fastcall__ newblk (unsigned char b);    /* An empty block after b (1: no room) */
void __fastcall__ delblk (unsigned char b);     /* Block b (empty) given back */
unsigned char split (void);                     /* The cursor's block (full) split (1: no room) */
unsigned char __fastcall__ fwd1 (unsigned fg);  /* The iterator past the next f or g (low, high byte; 1: none) */
void quiet (void);                              /* A note handler: every note ignored */
unsigned __fastcall__ uncr (unsigned char* s, unsigned n);  /* n bytes at s, CR LF made LF: their count now */
void __fastcall__ setbank (unsigned char b);    /* The bank at $8000: b.  (hydra.h's hy_bank, but a call: cc65's
                                                ** optimizer can lose an index in hy_bank (a[i])) */

/* text.c */
unsigned char t_new (void);                     /* The file shown: empty, a block (0, or 1: no bank) */
void t_free (void);                             /* Its banks given back */
int t_load (const char* name);                  /* Its text from a file: 0; -1 (_oserror); 1: no room */
int t_save (const char* name);                  /* Its text to a file: 0; -1 (_oserror) */
lpos t_len (void);                              /* The text's length */
unsigned t_lines (void);                        /* Its lines (an empty last line counted) */
void t_gotoline (unsigned n);                   /* The cursor to line n's start */
void t_bol (void);                              /* The cursor to its line's start */
void t_eol (void);                              /* ... and to its end (before the LF) */
unsigned char t_ins (const unsigned char* s, unsigned n);   /* n bytes in at the cursor, it after them (1: no room) */
void t_del (lpos n, unsigned char kind, void (*sink) (const unsigned char* s, unsigned n));
                                                /* n bytes after the cursor out (to sink), recorded as kind */
lpos t_find (const unsigned char* pat, unsigned char n, lpos from, signed char dir);
                                                /* pat (n bytes; letters either case) at or after from (dir 1), or at
                                                ** or before it (-1): its place, or NOWHERE */

/* undo.c */
#define U_TYPE      1                           /* ugroup: the last record is typing ... */
#define U_DEL       2                           /*   deleting forward ... */
#define U_RUB       3                           /*   rubbing out (backwards) ... */
#define U_PASTE     4                           /*   a paste (its parts one record) ... */
#define U_PASTE0    5                           /*   a paste's start */
unsigned char u_new (void);                     /* D's undo log: a bank (1: none) */
void u_clear (void);                            /* Nothing to undo or redo */
void u_ins (lpos p, const unsigned char* s, unsigned n);    /* Recorded: s put in at p */
void u_delstart (lpos p, lpos n, unsigned char kind);       /* Recorded: n bytes out at p (U_DEL, U_RUB) ... */
void u_delbytes (const unsigned char* s, unsigned n);       /*   these, as they go */
unsigned char u_undo (void);                    /* The last change undone (0: none) */
unsigned char u_redo (void);                    /* The last undone done again (0: none) */
extern unsigned char u_off;                     /* <> 0: nothing recorded (undo, redo themselves) */
lpos c_len (void);                              /* The cut buffer's length */
void c_clear (void);
unsigned char c_add (const unsigned char* s, unsigned n);   /* On the end of the cut buffer (1: no room) */
unsigned char c_paste (void);                   /* The cut buffer in at the cursor (1: no room) */

/* screen.c */
extern unsigned char W, H, TH;                  /* The screen: its width, height, and the text's rows */
extern unsigned char helpshown;                 /* The two help lines shown */
unsigned char s_init (void);                    /* (1: no room: no bank for the screen's copy) */
void s_done (void);
void s_all (void);                              /* Everything drawn again */
void s_resize (void);                           /* The window's size changed (CH_RESIZE): drawn again at it */
void s_render (void);                           /* The screen made to show D and the cursor */
void s_msg (const char* m);                     /* A message on the message line, till the next key */
void s_msg2 (const char* a, const char* b);     /* ... of two parts */
unsigned char s_prompt (const char* q, char* buf, unsigned char max);   /* A line typed (1: Enter, 0: Esc) */
int s_ask (const char* q, const char* keys);    /* One of keys typed (its index; -1: Esc) */
void s_help (void);                             /* The keys, the whole screen, till a key */
void s_dirty (void);                            /* The text's rows from the cursor's down to be drawn again */
void s_dirtyrow (void);                         /* ... the cursor's row alone */
void s_dirtyall (void);                         /* ... all of them */
void s_settop (unsigned line);                  /* The screen's top line: line (the cursor's on the screen) */
unsigned colof (void);                          /* The cursor's column */
void tocol (unsigned want);                     /* The cursor on its line, to column want (or its end) */
