/*
** db.c - db [-p task | program [arg ...]]: the debugger (phase 9).  A program started stopped at its entry point
** (in a task and note group of its own, so Ctrl-C reaches db, which stops it), or a task it attaches to (stopped
** where it is); then stepped an instruction at a time, run to a breakpoint or an address, its registers and memory
** shown and its memory changed, with symbols from ld65's label files (-Ln: "al 000830 .main") and hydra.inc's
** calls; an instruction as the assembler as writes it (the asm library's: asm.h, the same as dis's).  All through
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
**     l file         symbols from an ld65 label file (or hydra.inc: the calls' names)
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
#include <asm.h>

#define BP_MAX      16          /* Breakpoints, at most */
#define NEAR        0x100       /* A symbol names an address up to this far past it */

#define OP_BRK      0x00
#define OP_JSR      0x20

struct bp {
    unsigned        addr;
    unsigned char   byte;               /* Its own byte there, while the BRK's in */
    unsigned char   in;                 /* The BRK's in its memory */
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
static unsigned dnext, mnext;           /* Where d and m go on from */
static char line[128];
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

/* A line for the instruction at a: its address, its bytes, it as as writes it (a symbol for the address its
** operand names, if there's one near), and the symbol of where it is.  Its length */
static unsigned char showop (unsigned a)
{
    static struct dis d;
    static char opname[48];
    unsigned char n, i;
    const char* s;

    memset (buf, 0, 3);
    peek (a, buf, 3);
    d.at = a;
    d.bytes = buf;
    d.name = 0;
    d.flags = 0;
    if (!dis_insn (&d)) {
        puts ("db: no asm library");
        return 1;
    }
    if ((d.kind & (DK_ADDR | DK_GO)) && (s = lbl_near (d.addr, NEAR)) != 0) {
        strcpy (opname, s);
        d.name = opname;
        dis_insn (&d);
    }
    n = d.len;
    printf ("%04X  ", a);
    for (i = 0; i < 3; ++i) {
        if (i < n) {
            printf ("%02X ", buf[i]);
        } else {
            fputs ("   ", stdout);
        }
    }
    if ((s = lbl_near (a, NEAR)) != 0) {
        printf (" %-24s ; %s\n", d.text, s);
    } else {
        printf (" %s\n", d.text);
    }
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
    if (!dollar && lbl_addr (name, a) == 0) {
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
        } else if ((k = lbl_load (a1)) < 0) {
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
