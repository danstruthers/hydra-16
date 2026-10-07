/*
** db.c - db [-p task | program [arg ...]]: the debugger (phase 9).  A program started stopped at its entry point
** (in a task and note group of its own, so Ctrl-C reaches db, which stops it), or a task it attaches to (stopped
** where it is); then stepped an instruction at a time, run to a breakpoint or an address, its registers and memory
** shown and its memory changed, with symbols from ld65's label files (-Ln: "al 000830 .main").  All through
** /proc/N: ctl (stop, start, step, next, break, nobreak, kill) and mem; and the kernel's TASKREAD (its frame) and
** TASKINFO (its flags: stopped).
**   Its commands, a line each.  An address is hex ($ before it, or not) or a symbol, then + or - a hex offset; a
** count is decimal.
**     r              its registers, and the instruction at its PC
**     s [n]          step n instructions (1), into a JSR's subroutine
**     n [n]          the same, a JSR's subroutine run whole (a JSR to the kernel's jump table always is)
**     c              continue, till a breakpoint, a BRK, its end, or Ctrl-C
**     u addr         step (as n) till its PC is addr (or a breakpoint, or Ctrl-C)
**     b [addr]       a breakpoint at addr (a BRK written there while it runs: RAM only), or the list
**     x [addr]       the breakpoint at addr gone, or all of them
**     d [addr] [n]   n instructions (12) from addr (its PC; then on from the last)
**     m [addr] [n]   n bytes (64) from addr (then on from the last), hex and text
**     w addr byte..  bytes (hex) written at addr
**     l file         symbols from an ld65 label file
**     q              quit: a program db started is killed; a task it attached to runs on
**   While it's stopped db's breakpoints are out of its memory (d and m show its own bytes); they go in as it runs
** (c, u, and n over a JSR).  A step stops where the kernel can't step (in a call: E_BUSY): c runs it on.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <hydra.h>

#define BP_MAX      16          /* Breakpoints, at most */
#define SYM_GROW    64          /* The symbols' table grows this many at a time */
#define NEAR        0x100       /* A symbol names an address up to this far past it */

/* The modes (op_mode): implied, A, #, zp, zp,X, zp,Y, (zp,X), (zp),Y, (zp), abs, abs,X, abs,Y, (abs), (abs,X),
** rel, zp,rel (BBR, BBS); and the opcodes the W65C02S doesn't define: NOPs of 1, 2 and 3 bytes */
enum { M_IMP, M_ACC, M_IMM, M_ZP, M_ZPX, M_ZPY, M_IZX, M_IZY, M_IZP, M_ABS, M_ABX, M_ABY, M_IND, M_IAX, M_REL,
       M_ZPR, M_N1, M_N2, M_N3 };
static const unsigned char mode_len[] = { 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2, 3, 1, 2, 3 };

/* The mnemonics, 3 letters each (RMB, SMB, BBR and BBS: their bit after them, from the opcode) */
static const char names[] =
    "BRKORA???TSBASLRMBPHPBBRBPLTRBCLCINCJSRANDBITROL"
    "PLPBMISECDECRTIEORLSRPHAJMPBVCCLIPHYRTSADCSTZROR"
    "PLABVSSEIPLYBRASTASTYSTXSMBDEYTXABBSBCCTYATXSLDY"
    "LDALDXTAYTAXBCSCLVTSXCPYCMPINYDEXWAIBNECLDPHXSTP"
    "CPXSBCINXNOPBEQSEDPLX";

/* Each opcode's mnemonic (its index in names) and mode (M_*) */
static const unsigned char op_name[256] = {
     0,  1,  2,  2,  3,  1,  4,  5,  6,  1,  4,  2,  3,  1,  4,  7,
     8,  1,  1,  2,  9,  1,  4,  5, 10,  1, 11,  2,  9,  1,  4,  7,
    12, 13,  2,  2, 14, 13, 15,  5, 16, 13, 15,  2, 14, 13, 15,  7,
    17, 13, 13,  2, 14, 13, 15,  5, 18, 13, 19,  2, 14, 13, 15,  7,
    20, 21,  2,  2,  2, 21, 22,  5, 23, 21, 22,  2, 24, 21, 22,  7,
    25, 21, 21,  2,  2, 21, 22,  5, 26, 21, 27,  2,  2, 21, 22,  7,
    28, 29,  2,  2, 30, 29, 31,  5, 32, 29, 31,  2, 24, 29, 31,  7,
    33, 29, 29,  2, 30, 29, 31,  5, 34, 29, 35,  2, 24, 29, 31,  7,
    36, 37,  2,  2, 38, 37, 39, 40, 41, 14, 42,  2, 38, 37, 39, 43,
    44, 37, 37,  2, 38, 37, 39, 40, 45, 37, 46,  2, 30, 37, 30, 43,
    47, 48, 49,  2, 47, 48, 49, 40, 50, 48, 51,  2, 47, 48, 49, 43,
    52, 48, 48,  2, 47, 48, 49, 40, 53, 48, 54,  2, 47, 48, 49, 43,
    55, 56,  2,  2, 55, 56, 19, 40, 57, 56, 58, 59, 55, 56, 19, 43,
    60, 56, 56,  2,  2, 56, 19, 40, 61, 56, 62, 63,  2, 56, 19, 43,
    64, 65,  2,  2, 64, 65, 11, 40, 66, 65, 67,  2, 64, 65, 11, 43,
    68, 65, 65,  2,  2, 65, 11, 40, 69, 65, 70,  2,  2, 65, 11, 43
};

static const unsigned char op_mode[256] = {
     0,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15,
    14,  7,  8, 16,  3,  4,  4,  3,  0, 11,  1, 16,  9, 10, 10, 15,
     9,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15,
    14,  7,  8, 16,  4,  4,  4,  3,  0, 11,  1, 16, 10, 10, 10, 15,
     0,  6, 17, 16, 17,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15,
    14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0, 16, 18, 10, 10, 15,
     0,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16, 12,  9,  9, 15,
    14,  7,  8, 16,  4,  4,  4,  3,  0, 11,  0, 16, 13, 10, 10, 15,
    14,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15,
    14,  7,  8, 16,  4,  4,  5,  3,  0, 11,  0, 16,  9, 10, 10, 15,
     2,  6,  2, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15,
    14,  7,  8, 16,  4,  4,  5,  3,  0, 11,  0, 16, 10, 10, 11, 15,
     2,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0,  0,  9,  9,  9, 15,
    14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0,  0, 18, 10, 10, 15,
     2,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15,
    14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0, 16, 18, 10, 10, 15
};

#define OP_BRK      0x00
#define OP_JSR      0x20

struct bp {
    unsigned        addr;
    unsigned char   byte;               /* Its own byte there, while the BRK's in */
    unsigned char   in;                 /* The BRK's in its memory */
};

struct sym {
    unsigned        addr;
    char*           name;
};

static unsigned char task;              /* The task debugged ... */
static unsigned char started;           /*   started by db (else attached to) */
static int ctl, mem;                    /* Its /proc/N/ctl and mem */
static unsigned char frame[HY_TF_SIZE]; /* Its state and frame (TASKREAD) */
static unsigned char info[HY_TI_SIZE];  /* TASKINFO's */
static struct hy_regs r;
static volatile unsigned char interrupted;
static struct bp bps[BP_MAX];
static unsigned char nbp;
static struct sym* syms;
static unsigned nsym, symroom;
static unsigned dnext, mnext;           /* Where d and m go on from */
static char line[128];
static char text[48];                   /* An instruction's text, or a symbol's */
static unsigned char buf[16];

static void onint (int sig)
{
    (void) sig;
    interrupted = 1;
}

static const char* why (void)
{
    return _oserror ? _stroserror (_oserror) : strerror (errno);
}

/* /proc/N/name opened */
static int pfile (const char* name, int mode)
{
    sprintf (line, "/proc/%u/%s", task, name);
    return open (line, mode);
}

/* A line to its ctl: 0, or -1 (_oserror) */
static int cmd (const char* s)
{
    return write (ctl, s, strlen (s)) < 0 ? -1 : 0;
}

static unsigned pc (void)
{
    return frame[HY_TF_PC] | frame[HY_TF_PC + 1] << 8;
}

static void getframe (void)
{
    r.a = task;
    r.x = HY_TR_FRAME;
    r.r[0] = (unsigned) frame;
    hy_call (HY_TASKREAD, &r);
}

/* n bytes of its memory at a into b (through mem: an area at a time).  The bytes read */
static unsigned peek (unsigned a, unsigned char* b, unsigned n)
{
    unsigned got = 0;
    int k;

    while (got < n) {
        if (lseek (mem, (unsigned long) (unsigned) (a + got), SEEK_SET) < 0 ||
            (k = read (mem, b + got, n - got)) <= 0) {
            break;
        }
        got += k;
    }
    return got;
}

/* A byte written into its memory at a: 0, or -1 (a ROM: E_PERM) */
static int poke (unsigned a, unsigned char v)
{
    if (lseek (mem, (unsigned long) a, SEEK_SET) < 0 || write (mem, &v, 1) != 1) {
        return -1;
    }
    return 0;
}

/* ---- Symbols */

/* The symbol for address a: "name" or "name+off" (the nearest at or below it, NEAR at most), or 0 */
static const char* symname (unsigned a)
{
    unsigned i, best = 0xFFFF, d;
    struct sym* s = 0;

    for (i = 0; i < nsym; ++i) {
        if (syms[i].addr <= a && (d = a - syms[i].addr) < NEAR && d < best) {
            best = d;
            s = &syms[i];
        }
    }
    if (!s) {
        return 0;
    }
    if (best == 0) {
        return s->name;
    }
    sprintf (text, "%.36s+%X", s->name, best);
    return text;
}

/* An ld65 label file's symbols ("al 000830 .main"): the count taken, or -1 */
static int loadsyms (const char* file)
{
    static char lb[80];
    FILE* f = fopen (file, "r");
    char* p;
    char* e;
    unsigned n = 0;

    if (!f) {
        return -1;
    }
    while (fgets (lb, sizeof lb, f)) {
        if (lb[0] != 'a' || lb[1] != 'l' || lb[2] != ' ') {
            continue;
        }
        p = strchr (lb + 3, ' ');
        if (!p || p[1] != '.' || p[2] == '@' || p[2] == '_') {
            continue;
        }
        for (e = p; *e && *e != '\r' && *e != '\n'; ++e) {
        }
        *e = 0;
        if (nsym == symroom) {
            syms = realloc (syms, (symroom + SYM_GROW) * sizeof (struct sym));
            if (!syms) {
                fputs ("db: out of memory\n", stderr);
                exit (1);
            }
            symroom += SYM_GROW;
        }
        syms[nsym].addr = (unsigned) strtoul (lb + 3, 0, 16);
        if ((syms[nsym].name = strdup (p + 2)) == 0) {
            break;
        }
        ++nsym;
        ++n;
    }
    fclose (f);
    return n;
}

/* ---- Disassembly */

/* The instruction at a (its bytes in b) as text (in text: mnemonic and operand); its length */
static unsigned char disasm (unsigned a, const unsigned char* b)
{
    unsigned char op = b[0], mode = op_mode[op];
    unsigned w = b[1] | b[2] << 8;
    const char* nm = names + 3 * op_name[op];
    char* t = text;

    t[0] = nm[0];
    t[1] = nm[1];
    t[2] = nm[2];
    t += 3;
    if ((op & 7) == 7) {                                /* RMB, SMB, BBR, BBS: their bit */
        *t++ = '0' + (op >> 4 & 7);
    }
    *t++ = ' ';
    switch (mode) {
    case M_ACC: strcpy (t, "A"); break;
    case M_IMM: sprintf (t, "#$%02X", b[1]); break;
    case M_ZP:  sprintf (t, "$%02X", b[1]); break;
    case M_ZPX: sprintf (t, "$%02X,X", b[1]); break;
    case M_ZPY: sprintf (t, "$%02X,Y", b[1]); break;
    case M_IZX: sprintf (t, "($%02X,X)", b[1]); break;
    case M_IZY: sprintf (t, "($%02X),Y", b[1]); break;
    case M_IZP: sprintf (t, "($%02X)", b[1]); break;
    case M_ABS: sprintf (t, "$%04X", w); break;
    case M_ABX: sprintf (t, "$%04X,X", w); break;
    case M_ABY: sprintf (t, "$%04X,Y", w); break;
    case M_IND: sprintf (t, "($%04X)", w); break;
    case M_IAX: sprintf (t, "($%04X,X)", w); break;
    case M_REL: sprintf (t, "$%04X", a + 2 + (signed char) b[1]); break;
    case M_ZPR: sprintf (t, "$%02X,$%04X", b[1], a + 3 + (signed char) b[2]); break;
    default:    t[-1] = 0; break;
    }
    return mode_len[mode];
}

/* Where the instruction at a (its bytes in b) goes, if it names a place (JMP, JSR, a branch): its address, or 0 */
static unsigned target (unsigned a, const unsigned char* b)
{
    switch (op_mode[b[0]]) {
    case M_ABS: return (b[0] == 0x4C || b[0] == OP_JSR) ? (b[1] | b[2] << 8) : 0;
    case M_REL: return a + 2 + (signed char) b[1];
    case M_ZPR: return a + 3 + (signed char) b[2];
    }
    return 0;
}

/* A line for the instruction at a: its address, bytes, text, and the symbols of where it is and where it goes.
** Its length */
static unsigned char showop (unsigned a)
{
    static char ins[24];
    unsigned char n, i;
    unsigned to;
    const char* s;

    memset (buf, 0, 3);
    peek (a, buf, 3);
    n = disasm (a, buf);
    strcpy (ins, text);
    printf ("%04X  ", a);
    for (i = 0; i < 3; ++i) {
        if (i < n) {
            printf ("%02X ", buf[i]);
        } else {
            fputs ("   ", stdout);
        }
    }
    printf (" %-16s", ins);
    if ((s = symname (a)) != 0) {
        printf (" %s", s);
    }
    if ((to = target (a, buf)) != 0 && (s = symname (to)) != 0) {
        printf (" -> %s", s);
    }
    putchar ('\n');
    return n;
}

/* ---- Its state */

/* Its registers, and the instruction at its PC.  (P's B means nothing in a frame: a PHP pushes it set, an
** interrupt clear) */
static void regs (void)
{
    static const char fl[] = "NV--DIZC";
    char p[9];
    unsigned char i, v = frame[HY_TF_P];

    for (i = 0; i < 8; ++i) {
        p[i] = (v & 0x80 >> i) ? fl[i] : (fl[i] == '-' ? '-' : fl[i] | 0x20);
    }
    p[8] = 0;
    printf ("PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%s W=%X U=%X RAM=%02X ROM=%02X", pc (), frame[HY_TF_A],
        frame[HY_TF_X], frame[HY_TF_Y], frame[HY_TF_S], p, frame[HY_TF_W], frame[HY_TF_U], frame[HY_TF_RAM],
        frame[HY_TF_ROM]);
    if (frame[HY_TF_STATE] != 1) {
        printf (" (in a call: its state %u)", frame[HY_TF_STATE]);
    }
    putchar ('\n');
    dnext = pc ();
    dnext += showop (dnext);
}

/* ---- Breakpoints */

static int bpat (unsigned a)
{
    unsigned char i;

    for (i = 0; i < nbp; ++i) {
        if (bps[i].addr == a) {
            return i;
        }
    }
    return -1;
}

/* Every breakpoint's BRK into its memory, but at skip (its PC) */
static void bp_in (unsigned skip)
{
    unsigned char i;

    for (i = 0; i < nbp; ++i) {
        if (!bps[i].in && bps[i].addr != skip && peek (bps[i].addr, &bps[i].byte, 1) == 1 &&
            poke (bps[i].addr, OP_BRK) == 0) {
            bps[i].in = 1;
        }
    }
}

/* Every breakpoint's byte back */
static void bp_out (void)
{
    unsigned char i;

    for (i = 0; i < nbp; ++i) {
        if (bps[i].in) {
            poke (bps[i].addr, bps[i].byte);
            bps[i].in = 0;
        }
    }
}

/* ---- Running it */

/* It ended: its code and message (if db started it, and can wait for it); db ends too */
static void ended (void)
{
    static char msg[HY_EXIT_MSG_MAX + 1];
    int code;

    if (started && (code = hy_wait (task, msg)) >= 0) {
        printf ("task %u ended: code %d%s%s\n", task, code, msg[0] ? ", " : "", msg);
    } else {
        printf ("task %u ended\n", task);
    }
    exit (0);
}

/* Wait for it to stop (Ctrl-C stops it), and take its frame; if it ends, db does */
static void waitstop (void)
{
    for (;;) {
        r.a = task;
        r.r[0] = (unsigned) info;
        if (hy_call (HY_TASKINFO, &r) || info[HY_TI_STATE] == 0) {
            ended ();
        }
        if (info[HY_TI_FLAGS] & HY_TF_STOPPED) {
            break;
        }
        if (interrupted) {
            interrupted = 0;
            cmd ("stop\n");
        }
        hy_yield ();
    }
    getframe ();
}

/* Why a step was refused */
static void refused (void)
{
    if (_oserror == HY_E_BUSY) {
        puts ("it can't be stepped here: in a call, or a step over still running (c runs it on)");
    } else if (_oserror == HY_E_INVAL) {
        puts ("it can't be stepped at a BRK or STP (c: a BRK of its own is its note)");
    } else {
        printf ("db: %s\n", why ());
    }
}

/* One step ("step\n" or "next\n"): its breakpoints in over a JSR's subroutine.  0, or -1 (refused: said) */
static int step (const char* how)
{
    unsigned char op = 0;
    int k;

    peek (pc (), &op, 1);
    if (op == OP_JSR && how[0] == 'n') {
        bp_in (pc ());
    }
    k = cmd (how);
    if (k == 0) {
        waitstop ();
    }
    bp_out ();
    if (k < 0) {
        refused ();
    }
    return k;
}

/* Stopped after it ran: where, and why */
static void stopped (void)
{
    int i = bpat (pc ());
    unsigned char op = 1;

    bp_out ();
    if (i >= 0) {
        printf ("breakpoint %d\n", i + 1);
    } else if (peek (pc (), &op, 1) == 1 && op == OP_BRK) {
        puts ("a BRK of its own");
    }
    regs ();
}

/* c: on till it stops.  At a breakpoint, a step first (its own byte there); at a BRK of its own, its note */
static void cont (void)
{
    unsigned char op = 1;

    if (bpat (pc ()) >= 0 && step ("step\n") < 0) {
        return;
    }
    peek (pc (), &op, 1);
    if (op == OP_BRK) {
        cmd ("nobreak\n");
    }
    bp_in (0xFFFF);
    interrupted = 0;
    if (cmd ("start\n") < 0) {
        printf ("db: %s\n", why ());
        bp_out ();
        return;
    }
    waitstop ();
    if (op == OP_BRK) {
        cmd ("break\n");
    }
    stopped ();
}

/* ---- Commands */

/* An address: a symbol, or hex ($ before it: hex, even if a symbol has the name); then + or - a hex offset ($
** before it, or not).  1, or 0: not one (said) */
static unsigned char addr (const char* s, unsigned* a)
{
    static char name[40];
    const char* p;
    char* e;
    unsigned i, len;
    unsigned long off = 0;
    unsigned char minus = 0, dollar;

    if (!s) {
        return 0;
    }
    dollar = *s == '$';
    s += dollar;
    if (!*s) {
        return 0;
    }
    for (p = s + 1; *p && *p != '+' && *p != '-'; ++p) {
    }
    len = p - s;
    if (*p) {
        minus = *p == '-';
        p += p[1] == '$';
        off = strtoul (p + 1, &e, 16);
        if (*e || e == p + 1) {
            printf ("db: %s: not an offset\n", p);
            return 0;
        }
    }
    if (len >= sizeof name) {
        return 0;
    }
    memcpy (name, s, len);
    name[len] = 0;
    i = nsym;
    if (!dollar) {
        for (i = 0; i < nsym && strcmp (syms[i].name, name); ++i) {
        }
    }
    if (i < nsym) {
        *a = syms[i].addr;
    } else {
        *a = (unsigned) strtoul (name, &e, 16);
        if (*e) {
            printf ("db: %s: not an address, nor a symbol\n", name);
            return 0;
        }
    }
    *a = minus ? *a - (unsigned) off : *a + (unsigned) off;
    return 1;
}

/* A count (decimal), or dflt with none */
static unsigned count (const char* s, unsigned dflt)
{
    return s ? (unsigned) strtoul (s, 0, 10) : dflt;
}

static void help (void)
{
    puts ("r  registers         s [n]  step           n [n]  step over a JSR");
    puts ("c  continue          u addr  until addr    b [addr]  breakpoint (list)");
    puts ("x [addr]  clear (all)    d [addr] [n]  disassemble    m [addr] [n]  memory");
    puts ("w addr byte..  write   l file  ld65 labels    q  quit");
}

/* m: n bytes from a, 8 a line */
static void dump (unsigned a, unsigned n)
{
    unsigned char i, k;

    while (n && !interrupted) {
        k = n < 8 ? n : 8;
        k = peek (a, buf, k);
        if (k == 0) {
            break;
        }
        printf ("%04X ", a);
        for (i = 0; i < 8; ++i) {
            if (i < k) {
                printf (" %02X", buf[i]);
            } else {
                fputs ("   ", stdout);
            }
        }
        fputs ("  ", stdout);
        for (i = 0; i < k; ++i) {
            putchar (buf[i] >= ' ' && buf[i] < 0x7F ? buf[i] : '.');
        }
        putchar ('\n');
        a += k;
        n -= k;
    }
    mnext = a;
}

/* One command line.  0: quit */
static unsigned char command (char* s)
{
    char* c = strtok (s, " \t\n");
    char* a1 = strtok (0, " \t\n");
    char* a2 = strtok (0, " \t\n");
    unsigned a, n, i;
    int k;

    interrupted = 0;
    if (!c) {
        return 1;
    }
    switch (*c) {
    case 'r':
        regs ();
        break;

    case 's':
    case 'n':
        n = count (a1, 1);
        while (n-- && !interrupted) {
            if (step (*c == 's' ? "step\n" : "next\n") < 0) {
                break;
            }
            if ((k = bpat (pc ())) >= 0) {
                printf ("breakpoint %d\n", k + 1);
                break;
            }
            if (n) {
                showop (pc ());
            }
        }
        regs ();
        break;

    case 'c':
        cont ();
        break;

    case 'u':
        if (!addr (a1, &a)) {
            puts ("u addr");
            break;
        }
        while (pc () != a && !interrupted) {
            if (step ("next\n") < 0 || bpat (pc ()) >= 0) {
                break;
            }
        }
        stopped ();
        break;

    case 'b':
        if (!a1) {
            for (i = 0; i < nbp; ++i) {
                printf ("%u ", i + 1);
                showop (bps[i].addr);
            }
        } else if (!addr (a1, &a)) {
            puts ("b addr");
        } else if (bpat (a) >= 0) {
            puts ("there's one there");
        } else if (nbp == BP_MAX) {
            puts ("no more breakpoints");
        } else if (a >= 0xA000) {
            puts ("not in RAM: no breakpoint");
        } else {
            bps[nbp].addr = a;
            bps[nbp].in = 0;
            ++nbp;
        }
        break;

    case 'x':
        if (!a1) {
            nbp = 0;
        } else if (!addr (a1, &a) || (k = bpat (a)) < 0) {
            puts ("no breakpoint there");
        } else {
            bps[k] = bps[--nbp];
        }
        break;

    case 'd':
        a = dnext;
        if (a1 && !addr (a1, &a)) {
            break;
        }
        for (n = count (a2, 12); n-- && !interrupted; ) {
            a += showop (a);
        }
        dnext = a;
        break;

    case 'm':
        a = mnext;
        if (a1 && !addr (a1, &a)) {
            break;
        }
        dump (a, count (a2, 64));
        break;

    case 'w':
        if (!addr (a1, &a)) {
            puts ("w addr byte ...");
            break;
        }
        for (s = a2; s; s = strtok (0, " \t\n")) {
            if (poke (a++, (unsigned char) strtoul (s, 0, 16)) < 0) {
                printf ("db: %04X: %s\n", a - 1, why ());
                break;
            }
        }
        break;

    case 'l':
        if (!a1) {
            puts ("l file");
        } else if ((k = loadsyms (a1)) < 0) {
            printf ("db: %s: %s\n", a1, why ());
        } else {
            printf ("%d symbols\n", k);
        }
        break;

    case 'q':
        return 0;

    default:
        help ();
        break;
    }
    return 1;
}

int main (int argc, char* argv[])
{
    static char path[HY_PATH_MAX + 1];
    int t;

    if (argc < 2 || (argv[1][0] == '-' && (strcmp (argv[1], "-p") || argc != 3))) {
        fputs ("usage: db [-p task | program [arg ...]]\n", stderr);
        hy_exits ("usage");
    }
    signal (SIGINT, onint);
    if (argv[1][0] == '-') {                            /* -p: a task, stopped where it is */
        task = atoi (argv[2]);
    } else {                                            /* A program: stopped at its entry point */
        if (strchr (argv[1], '/')) {
            strncpy (path, argv[1], HY_PATH_MAX);
        } else {
            sprintf (path, "/bin/%.58s", argv[1]);
        }
        if ((t = hy_spawn (path, argv + 1, HY_SPAWN_STOPPED | HY_SPAWN_NEWGROUP)) < 0) {
            fprintf (stderr, "db: %s: %s\n", path, why ());
            hy_exits ("not started");
        }
        task = t;
        started = 1;
    }
    if ((ctl = pfile ("ctl", O_WRONLY)) < 0 || (mem = pfile ("mem", O_RDWR)) < 0) {
        fprintf (stderr, "db: %s: %s\n", line, why ());
        hy_exits ("no task");
    }
    if (!started && cmd ("stop\n") < 0) {
        fprintf (stderr, "db: task %u: %s\n", task, why ());
        hy_exits ("not stopped");
    }
    waitstop ();
    cmd ("break\n");
    printf ("task %u\n", task);
    regs ();
    for (;;) {
        fputs ("db> ", stdout);
        fflush (stdout);
        if (!fgets (line, sizeof line, stdin)) {
            if (interrupted) {
                clearerr (stdin);
                putchar ('\n');
                continue;
            }
            break;
        }
        if (!command (line)) {
            break;
        }
    }
    bp_out ();
    if (started) {                                      /* Quit: its end, or it runs on */
        cmd ("kill\n");
        hy_wait (task, 0);
    } else {
        cmd ("nobreak\n");
        cmd ("start\n");
    }
    return 0;
}
