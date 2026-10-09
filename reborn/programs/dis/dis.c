/*
** dis.c - dis [-cnw] [-l labels] [-o addr] file: the disassembler, the assembler's inverse.  A program (a HYX2 RAM
** program, as as and ld65 make them; or with -o, raw bytes from addr, as as -b makes them) written on stdout as a
** source in as's language, which as assembles to the same bytes.  Each instruction is the asm library's (asm.h: db's
** and HyForth's disasm's way too).
**   Its code is found by following it from where it starts (a program's main; -o's addr): an instruction to the next,
** and to where it jumps, branches or calls, till one that goes no further (rts, jmp ...); what isn't reached is data
** (.byte, its text in quotes).  -c: every byte from the start an instruction, in turn.  A place an instruction names in
** it is a label, its name its symbol's from -l's label file (as -l's, ld65 -Ln's), else Lnnnn (main at a program's
** entry); its zero page's and other places' symbols are defined first (name = $nn), its BSS's are labels in .bss; the
** system calls and r0-r15 are hydra.inc's names (/lib/as/hydra.inc: -n, not read).  A program's header is
** HYX2_PROGRAM's where that makes the same bytes (as's own programs'), else its bytes; its data .data, its BSS .bss.
**   Its lines are compact (a tab, then an instruction, one space before its operand: a 20K program's source some
** 100K, which as reads into its RAM banks whole); -w, the SDK's sources' columns.  The image and a flags byte for
** each of its bytes are in the task's RAM banks (banks.s), so a program of the whole $0800-$7FFF fits.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>
#include <asm.h>

#define F_START     0x01        /* An instruction starts here (found by following the code) ... */
#define F_LEN       0x06        /*   its length (1-3), shifted 1 */
#define F_WANT      0x08        /* A label here */
#define F_LINE      0x10        /* A line starts here ... */
#define F_DATA      0x20        /*   a line of data */
#define F_TODO      0x40        /* To be followed (the stack was full) */

#define TODO_MAX    256         /* Places to follow the code from, waiting */
#define SYMS_MAX    128         /* Symbols outside the image (definitions), and labels in the BSS */
#define HDR         48          /* A HYX2 header */
#define DATA_LINE   16          /* A line of data's bytes, at most */
#define BANK        0x2000

extern unsigned bk_base;
extern unsigned char bk_img, bk_flg, bk_bits;
unsigned char __fastcall__ img (unsigned a);
unsigned char* __fastcall__ img_ptr (unsigned a);
unsigned char __fastcall__ flg (unsigned a);
void __fastcall__ flg_or (unsigned a);

static const char* file;
static unsigned base, end;              /* The image: from base to end */
static unsigned cstart, cend;           /* Its code: from its start (past a header) to its data */
static unsigned bss, bssend;            /* A program's BSS */
static unsigned entry;                  /* Where it starts */
static unsigned char hyx, macro, all, nosys, wide;
static const char* ind = "\t";         /* A line's indent, and the room after a directive (-w: the SDK's columns) */
static const char* dir = " ";
static unsigned char hdr[HDR];
static struct dis d;
static unsigned char b3[3];
static unsigned todo[TODO_MAX];
static unsigned ntodo;
static unsigned ext[SYMS_MAX];          /* Symbols outside it its instructions name: their addresses ... */
static unsigned next;
static unsigned bl[SYMS_MAX];           /*   and its BSS's labels', in order */
static unsigned nbl;
static char name[64], name2[64];
static unsigned char buf[512];

static void usage (void)
{
    fputs ("usage: dis [-cnw] [-l labels] [-o addr] file\n", stderr);
    exit (1);
}

static void fail (const char* what, const char* why)
{
    fprintf (stderr, "dis: %s: %s\n", what, why);
    exit (1);
}

static const char* why (void)
{
    return _oserror ? _stroserror (_oserror) : strerror (errno);
}

static unsigned word (unsigned char i)
{
    return hdr[i] | hdr[i + 1] << 8;
}

static void set (unsigned a, unsigned char bits)
{
    bk_bits = bits;
    flg_or (a);
}

/* The instruction at a, into d (name: its operand's address's, or NULL) */
static void decode (unsigned a, const char* nm)
{
    b3[0] = img (a);
    b3[1] = a + 1 < end ? img (a + 1) : 0;
    b3[2] = a + 2 < end ? img (a + 2) : 0;
    d.at = a;
    d.bytes = b3;
    d.name = nm;
    d.flags = wide ? DF_PAD : 0;
    if (!dis_insn (&d)) {
        fail ("asm", "no asm library");
    }
}

/* ---- Symbols */

/* Is s (at a) hydra.inc's: a call's, r0-r15? */
static unsigned char sysname (const char* s, unsigned a)
{
    return !nosys && (a >= 0xF800 || (s[0] == 'r' && s[1] >= '0' && s[1] <= '9' && a < 0x22));
}

/* x (n of them, in order) with a in its place: 0, or -1 if there's no room */
static int insert (unsigned* x, unsigned* n, unsigned a)
{
    unsigned i, j;

    for (i = 0; i < *n && x[i] < a; ++i) {
    }
    if (i < *n && x[i] == a) {
        return 0;
    }
    if (*n == SYMS_MAX) {
        return -1;
    }
    for (j = *n; j > i; --j) {
        x[j] = x[j - 1];
    }
    x[i] = a;
    ++*n;
    return 0;
}

/* The symbol at a, or less than within before it, whose name names one place: a as an operand ("name", "name+2":
** name2), its address (*at), or NULL */
static const char* symnear (unsigned a, unsigned within, unsigned* at)
{
    const char* s = lbl_near (a, within);
    const char* p;

    if (!s) {
        return 0;
    }
    *at = a;
    if ((p = strchr (s, '+')) != 0) {
        *at = a - (unsigned) strtoul (p + 1, 0, 16);
    }
    if ((s = lbl_label (*at)) == 0 || strlen (s) > 40) {
        return 0;
    }
    strcpy (name2, s);
    if (*at != a) {
        sprintf (name2 + strlen (name2), "+%u", a - *at);
    }
    return name2;
}

/* An address an instruction names, outside the image: its BSS's (a label there), or a symbol's (defined) */
static void note (unsigned a)
{
    unsigned at;
    const char* s;

    if (a >= base && a < end) {
        return;
    }
    if (hyx && bss == end && a >= bss && a < bssend) {
        if ((s = symnear (a, 0x100, &at)) == 0 || at < bss) {
            at = a;
        }
        insert (bl, &nbl, at);
        return;
    }
    if ((s = symnear (a, a < 0x100 ? 2 : 1, &at)) != 0 && !sysname (s, at)) {
        insert (ext, &next, at);
    }
}

/* The name of the label at a line's start a (into buf n): its symbol's, main (a program's entry), or Lnnnn */
static char* label (unsigned a, char* n)
{
    const char* s = lbl_label (a);

    if (s && strlen (s) < 40) {
        strcpy (n, s);
    } else if (hyx && a == entry) {
        strcpy (n, "main");
    } else {
        sprintf (n, "L%04X", a);
    }
    return n;
}

/* A BSS label's name: its symbol's, or Bnnnn */
static char* bsslabel (unsigned a, char* n)
{
    const char* s = lbl_label (a);

    if (s && strlen (s) < 40) {
        strcpy (n, s);
    } else {
        sprintf (n, "B%04X", a);
    }
    return n;
}

/* An address an instruction names, as its operand: a label in the image (or a label + its offset), in its BSS, a
** symbol, or NULL (hex) */
static const char* opname (unsigned t)
{
    unsigned s, at;
    unsigned i;
    const char* p;

    if (t >= base && t < end) {
        if (t < cstart) {
            return 0;
        }
        for (s = t; s > cstart && !(flg (s) & F_LINE); --s) {
        }
        if (!(flg (s) & F_WANT)) {
            return 0;
        }
        label (s, name);
        if (s != t) {
            sprintf (name + strlen (name), "+%u", t - s);
        }
        return name;
    }
    if (hyx && bss == end && t >= bss && t < bssend) {
        for (i = nbl; i > 0 && bl[i - 1] > t; --i) {
        }
        if (!i) {
            return 0;
        }
        bsslabel (bl[i - 1], name);
        if (bl[i - 1] != t) {
            sprintf (name + strlen (name), "+%u", t - bl[i - 1]);
        }
        return name;
    }
    if ((p = symnear (t, t < 0x100 ? 2 : 1, &at)) == 0) {
        return 0;
    }
    if (!sysname (p, at)) {
        for (i = 0; i < next && ext[i] != at; ++i) {
        }
        if (i == next) {
            return 0;
        }
    }
    strcpy (name, p);
    return name;
}

/* ---- Following the code */

static void follow (unsigned a)
{
    if (ntodo < TODO_MAX) {
        todo[ntodo++] = a;
    } else {
        set (a, F_TODO);                            /* (Found again when the stack's empty) */
    }
}

/* The instruction at a, marked: its start and length; a label where it goes (followed), and where it reads and
** writes, in the image; a symbol it names noted.  OUT: its kind (DK_UNDEF: not code) */
static unsigned char mark (unsigned a)
{
    unsigned at;

    decode (a, 0);
    if (d.kind & DK_UNDEF && !all) {
        return d.kind;
    }
    if (a + d.len > cend) {
        return DK_UNDEF;
    }
    set (a, F_START | d.len << 1);
    if (d.kind & (DK_GO | DK_ADDR)) {
        if (d.addr >= cstart && d.addr < end && (d.kind & DK_GO || d.addr >= 0x100)) {
            if (d.kind & DK_GO || !symnear (d.addr, 8, &at) || at < cstart) {
                at = d.addr;                        /* (Data just past a symbol: the symbol's label + its offset) */
            }
            set (at, F_WANT);
            if (d.kind & DK_GO && d.addr < cend && !(flg (d.addr) & F_START)) {
                follow (d.addr);
            }
        } else {
            note (d.addr);
        }
    }
    return d.kind;
}

static void trace (void)
{
    unsigned a;

    if (all) {
        for (a = cstart; a < cend; a += d.len) {
            if (mark (a) & DK_UNDEF && !(flg (a) & F_START)) {
                break;
            }
        }
        return;
    }
    follow (entry);
    do {
        while (ntodo) {
            for (a = todo[--ntodo]; a >= cstart && a < cend && !(flg (a) & F_START); a += d.len) {
                if (mark (a) & (DK_UNDEF | DK_END)) {
                    break;
                }
            }
        }
        for (a = cstart; a < cend && ntodo < TODO_MAX; ++a) {   /* (Those the stack had no room for) */
            if ((flg (a) & (F_TODO | F_START)) == F_TODO) {
                follow (a);
            }
        }
    } while (ntodo);
}

/* ---- Lines */

/* Does the instruction at a (n bytes) stand alone: no other's start, no label, nor the data's start in it? */
static unsigned char fits (unsigned a, unsigned char n)
{
    unsigned char k;

    if (a + n > end) {
        return 0;
    }
    for (k = 1; k < n; ++k) {
        if (flg (a + k) & (F_START | F_WANT) || a + k == cend) {
            return 0;
        }
    }
    return 1;
}

/* Each line's start: an instruction's that stands alone, else data's (to the next start, label, or the data's start;
** DATA_LINE at most); a label in a line made its start's + an offset */
static void plan (void)
{
    unsigned a, s;
    unsigned char f, n;

    for (a = cstart; a < end; ) {
        f = flg (a);
        n = (f & F_LEN) >> 1;
        if (f & F_START && fits (a, n)) {
            set (a, F_LINE);
            a += n;
            continue;
        }
        set (a, F_LINE | F_DATA);
        for (n = 1; n < DATA_LINE && a + n < end && a + n != cend && !(flg (a + n) & (F_START | F_WANT)); ++n) {
        }
        a += n;
    }
    for (a = cstart; a < end; ++a) {
        if ((flg (a) & (F_WANT | F_LINE)) == F_WANT) {
            for (s = a; !(flg (s) & F_LINE); --s) {
            }
            set (s, F_WANT);
        }
    }
}

/* ---- Writing it */

static void hex (unsigned v)
{
    printf (v < 0x100 ? "$%02X" : "$%04X", v);
}

/* A line of data: n bytes at a, text in quotes (4 or more printable characters), the rest in hex */
static void data (unsigned a, unsigned char n)
{
    unsigned char i, j, c, first = 1;

    printf ("%s.byte%s", ind, dir);
    for (i = 0; i < n; i = j) {
        for (j = i; j < n && (c = img (a + j)) >= ' ' && c < 0x7F && c != '"'; ++j) {
        }
        if (!first) {
            fputs (", ", stdout);
        }
        first = 0;
        if (j - i >= 4) {
            putchar ('"');
            for (; i < j; ++i) {
                putchar (img (a + i));
            }
            putchar ('"');
        } else {
            j = i + 1;
            printf ("$%02X", img (a + i));
        }
    }
    putchar ('\n');
}

static void header (void)
{
    unsigned char i;

    if (macro) {
        printf (".include \"hyx2.inc\"\n\n%sHYX2_PROGRAM \"%s\", %s\n", ind, (char*) hdr + 36, label (entry, name));
        return;
    }
    puts (".segment \"HEADER\"                                   ; (Its HYX2 header, as it is)");
    for (i = 0; i < HDR; ++i) {
        if (i % 12) {
            printf (", $%02X", hdr[i]);
        } else {
            printf ("%s.byte%s$%02X", ind, dir, hdr[i]);
        }
        if (i % 12 == 11) {
            putchar ('\n');
        }
    }
}

static void write_out (void)
{
    unsigned a, i, prev;
    unsigned char f;
    const char* s;

    printf ("; %s, as dis read it: as assembles this to its bytes again\n", file);
    if (!nosys || hyx) {
        puts (".include \"hydra.inc\"");
    }
    for (i = 0; i < next; ++i) {                    /* (Symbols outside it, before they're used: the zero page's) */
        printf ("%-15s = ", lbl_label (ext[i]));
        hex (ext[i]);
        putchar ('\n');
    }
    putchar ('\n');
    if (hyx) {
        header ();
    } else {
        printf (".org $%04X\n", base);
    }
    puts (".code");
    for (a = cstart; a < end; ) {
        if (a == cend) {
            puts (".data");
        }
        f = flg (a);
        if (f & F_WANT) {
            printf ("%s:\n", label (a, name2));
        }
        if (!(f & F_DATA)) {
            decode (a, 0);
            if (d.kind & (DK_GO | DK_ADDR) && (s = opname (d.addr)) != 0) {
                decode (a, s);
            }
            printf ("%s%s\n", ind, d.text);
            a += d.len;
        } else {
            for (i = 1; a + i < end && !(flg (a + i) & F_LINE); ++i) {
            }
            data (a, i);
            a += i;
        }
    }
    if (hyx && bss == end && bssend > bss) {
        puts (".bss");
        for (prev = bss, i = 0; i < nbl; prev = bl[i++]) {
            if (bl[i] > prev) {
                printf ("%s.res%s%u\n", ind, dir, bl[i] - prev);
            }
            printf ("%s:\n", bsslabel (bl[i], name));
        }
        printf ("%s.res%s%u\n", ind, dir, bssend - prev);
    }
}

/* ---- Reading it */

/* The file into the image's banks (base on), its length (len) */
static void load (int fd, unsigned len)
{
    unsigned off, n, got;
    int k;
    unsigned char nb = (len + BANK - 1) / BANK, b;

    if ((k = hy_banks_alloc (2 * nb)) < 0) {
        fail (file, "no RAM banks");
    }
    bk_img = k;
    bk_flg = k + nb;
    bk_base = base;
    for (b = 0; b < nb; ++b) {
        hy_bank (bk_flg + b);
        memset (HY_BANK_WINDOW, 0, BANK);
    }
    for (off = 0; off < len; off += n) {
        n = len - off < sizeof buf ? len - off : sizeof buf;
        got = 0;
        if (off < HDR && hyx) {
            memcpy (buf, hdr, HDR);
            got = HDR;
        }
        while (got < n) {                               /* (A read may give less: /pc's do) */
            if ((k = read (fd, buf + got, n - got)) <= 0) {
                fail (file, k < 0 ? why () : "too short");
            }
            got += k;
        }
        memcpy (img_ptr (base + off), buf, n);          /* (Its place, by banks.s: see img_ptr) */
    }
}

int main (int argc, char* argv[])
{
    int fd, i;
    const char* lfile = 0;
    unsigned org = 0, len;
    unsigned char raw = 0;
    long size;

    for (i = 1; i < argc && argv[i][0] == '-'; ++i) {
        if (!strcmp (argv[i], "-l") && i + 1 < argc) {
            lfile = argv[++i];
        } else if (!strcmp (argv[i], "-o") && i + 1 < argc) {
            raw = 1;
            ++i;
            org = (unsigned) strtoul (argv[i] + (argv[i][0] == '$'), 0, 16);
        } else if (strspn (argv[i] + 1, "cnw") == strlen (argv[i] + 1) && argv[i][1]) {
            all |= strchr (argv[i], 'c') != 0;
            nosys |= strchr (argv[i], 'n') != 0;
            wide |= strchr (argv[i], 'w') != 0;
        } else {
            usage ();
        }
    }
    if (i != argc - 1) {
        usage ();
    }
    file = argv[i];
    if (wide) {
        ind = "            ";
        dir = "       ";
    }
    if (!nosys) {
        lbl_load ("/lib/as/hydra.inc");
    }
    if (lfile && lbl_load (lfile) < 0) {
        fail (lfile, why ());
    }
    if ((fd = open (file, O_RDONLY)) < 0) {
        fail (file, why ());
    }
    if (raw) {
        if ((size = lseek (fd, 0, SEEK_END)) < 0 || lseek (fd, 0, SEEK_SET) < 0) {
            fail (file, why ());
        }
        if (size == 0 || size > 0x10000L - org) {
            fail (file, "too long for its address");
        }
        base = cstart = entry = org;
        len = (unsigned) size;
        end = cend = base + len;
    } else {
        if (read (fd, hdr, HDR) != HDR || memcmp (hdr, "HYX2", 4) || hdr[4] != HDR || word (8) != 0x0800) {
            fail (file, "not a HYX2 RAM program (-o addr: raw bytes from addr)");
        }
        hyx = 1;
        base = 0x0800;
        len = word (10);
        end = base + len;
        cstart = base + HDR;
        cend = word (12) >= cstart && word (12) <= end ? word (12) : end;
        bss = word (18);
        bssend = bss + word (20);
        entry = word (24);
        macro = hdr[5] == HY_HT_PROGRAM && hdr[6] == 0 && hdr[7] == HY_ABI_VERSION && word (14) == word (12) &&
            word (18) == word (12) + word (16) && word (22) == bssend && len == word (12) + word (16) - base &&
            !word (26) && !word (28) && !word (30) && !hdr[32] && hdr[33] == 1 && word (34) == 1 &&
            entry >= cstart && entry < cend && hdr[36] && !hdr[47];
        for (i = 36; macro && i < 47 && hdr[i]; ++i) {
            macro = hdr[i] > ' ' && hdr[i] < 0x7F && hdr[i] != '"';
        }
        for (; macro && i < HDR; ++i) {
            macro = !hdr[i];
        }
    }
    load (fd, len);
    close (fd);
    if (hyx && entry >= cstart && entry < end) {
        set (entry, F_WANT);
    }
    trace ();
    plan ();
    write_out ();
    return 0;
}
