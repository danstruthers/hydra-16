/*
** meta.c - libdq0meta: the helpers the programs between dungeons share (ENGINE-CRITIQUE.md's section 9, step 11:
** dq0life, dq0acct, dq0town, dq0schol, dq0end, themec, themegen, and the UI's hub_run).  A C source compiled into each
** program that wants it, not a library linked: each program takes the sections it uses and puts each where it wants it
** (its main RAM, or an overlay: the #pragmas it's included under), and the names it reads (its path, its buffers) are
** its own, declared before it's included.  So nothing is in a program that it doesn't call, and each one's bytes are
** where they were.  Included once for each section or run of sections:
**
**   #define META_WORDS
**   #include "../dq0/meta/meta.c"
**
** with no include guard (a section's #define taken back after it: #undef).  The sections:
**
**   META_FAIL      fail (why): "NAME: why" on fd 2, exit 1 (META_NAME the program's name, a string)
**   META_PATH      in_ (folder, name): folder/name in the program's path[] (its size the program's), its address
**   META_NUM       num (s): a number's digits, an unsigned long
**   META_SEED      seed_of (s, n): dungeon n's seed by the life's (dq0life's, the rumours' too)
**   META_WORDS     the words, wrapped by the console's width: say (s), sayn (v), amount (farthings: "22d 1f"),
**                  say_flush (); width, ob[META_OB] (the words' buffer, out as it fills), a word at most META_WB
**                  letters; PENNY the program's
**   META_WIDTH     width_read (): the console's width, /dev/consctl's
**   META_ASK       ask (prompt): a line typed into ans[] (the program's), lower case, its spaces trimmed, asks
**                  counted (the program's asks)
**   META_KEYS      words_of (key, n): the n'th line of key in a "key | value | value" text (tw[], twn: the
**                  program's), its values in v[] (nv); words_read (fd): such a text read, its notes (# lines) left
**                  out, through META_LINE (default lb[]) up to META_TEXT_MAX bytes; META_TEXT_ON () and
**                  META_TEXT_OFF () round each reading of tw (a bank put in and taken out: dq0town's)
**   META_VALUE     value_of (k, key[, n]): rumour k's line of key (rbuf[rat[k]] to rbuf[rto[k]]: the program's), its
**                  first value (with META_VALUE_N, its n'th), its indented lines joined, into said[]: 1, or 0 none;
**                  META_TEXT_ON/OFF as above
**   META_LAUNCH    the program launcher, the system's SPAWN and WAIT themselves (hy_spawn's argv wants the heap and
**                  stdio): META_SPAWN (path, args) a program started, its arguments each 0-ended, an empty one after
**                  them: its task, or -1; META_JOIN (task) waited for: its status, or -1 (the names meta_spawn,
**                  meta_join unless the program names them, META_SPAWN and META_JOIN: the UI's hub_run, hub_join)
**   META_RUN       meta_run (path, args): started and waited for: its status, or -1 (META_LAUNCH's two with it)
**   META_ARG       arg1 (q, s): an argument at q, the next's place; arg (q, flag, v) two
**   META_ARGN      argn (q, flag, v): a flag and a number
**
** META_LINK, empty or static (the default): what the sections define, shared with the program's other files or not.
*/

#ifndef META_LINK
#define META_LINK       static
#endif

#ifdef META_FAIL
META_LINK void __fastcall__ fail (const char* why)
{
    write (2, META_NAME ": ", sizeof META_NAME + 1);
    write (2, why, strlen (why));
    write (2, "\n", 1);
    exit (1);
}
#endif

#ifdef META_PATH
META_LINK const char* __fastcall__ in_ (const char* folder, const char* name)
{
    strcpy (path, folder);
    strcat (path, "/");
    strcat (path, name);
    return path;
}
#endif

#ifdef META_NUM
META_LINK unsigned long __fastcall__ num (const char* s)
{
    unsigned long n = 0;

    for (; *s >= '0' && *s <= '9'; ++s) {
        n = n * 10 + *s - '0';
    }
    return n;
}
#endif

#ifdef META_SEED
/* Dungeon n's seed, by the life's: the life's own the first's (a game begun with dq0 -s has that seed's dungeon); a seed under
** 65,536 goes on with seeds under it (the stand-in themes': main.c's history), one over goes on over it */
META_LINK unsigned long __fastcall__ seed_of (unsigned long s, unsigned n)
{
    unsigned long x;

    if (n <= 1) {
        return s;
    }
    x = s + n * 2654435761UL;
    x ^= x >> 15;
    x *= 40503UL;
    x ^= x >> 13;
    return s < 65536UL ? x & 0xFFFF : x | 0x10000UL;
}
#endif

#ifdef META_WORDS
/* ---- Words, by the console's width: a word kept till the space after it, then on this line or the next */

META_LINK unsigned char width = 79;
META_LINK char ob[META_OB];
static unsigned char col;
static char wb[META_WB];
static unsigned char wn, on;

static void out_flush (void)
{
    if (on) {
        write (1, ob, on);
        on = 0;
    }
}

static void __fastcall__ out_c (char c)
{
    if (on == sizeof ob) {
        out_flush ();
    }
    ob[on++] = c;
}

static void word_out (void)
{
    unsigned char k;

    if (!wn) {
        return;
    }
    if (col && col + wn + 1 > width) {
        out_c ('\n');
        col = 0;
    } else if (col) {
        out_c (' ');
        ++col;
    }
    for (k = 0; k < wn; ++k) {
        out_c (wb[k]);
    }
    col += wn;
    wn = 0;
}

META_LINK void __fastcall__ say (const char* s)
{
    for (; *s; ++s) {
        if (*s == ' ') {
            word_out ();
        } else if (*s == '\n') {
            word_out ();
            out_c ('\n');
            col = 0;
        } else if (wn < sizeof wb) {
            wb[wn++] = *s;
        }
    }
}

META_LINK void __fastcall__ sayn (unsigned long v)
{
    char b[12];

    say (ultoa (v, b, 10));
}

/* An amount, in farthings: "22d", "22d 1f", "3f" */
META_LINK void __fastcall__ amount (unsigned long f)
{
    if (f >= PENNY || !f) {
        sayn (f / PENNY);
        say ("d");
    }
    if (f % PENNY) {
        if (f >= PENNY) {
            say (" ");
        }
        sayn (f % PENNY);
        say ("f");
    }
}

/* The words so far out on the screen */
META_LINK void say_flush (void)
{
    word_out ();
    out_flush ();
}
#endif

#ifdef META_WIDTH
/* The console's width */
META_LINK void width_read (void)
{
    static char b[100];
    char* p;
    int fd, n = 0;

    if ((fd = open ("/dev/consctl", O_RDONLY)) >= 0) {
        n = read (fd, b, sizeof b - 1);
        close (fd);
    }
    b[n > 0 ? n : 0] = 0;
    for (p = b; p; p = strchr (p, '\n')) {
        if (*p == '\n') {
            ++p;
        }
        if (!strncmp (p, "size ", 5) && atoi (p + 5) >= 40) {
            width = atoi (p + 5) > 80 ? 79 : atoi (p + 5) - 1;
        }
    }
}
#endif

#ifdef META_ASK
/* A line typed, lower case, its spaces trimmed (ans): asked on a line of its own, then the game's "> " */
META_LINK void __fastcall__ ask (const char* prompt)
{
    int k;
    char* s;

    say (prompt);
    say ("\n");
    out_flush ();
    write (1, "> ", 2);
    ans[0] = 0;
    if ((k = read (0, ans, sizeof ans - 1)) > 0) {
        ans[k] = 0;
        ans[strcspn (ans, "\r\n")] = 0;
    }
    col = 0;
    for (s = ans; *s; ++s) {
        if (*s >= 'A' && *s <= 'Z') {
            *s |= 0x20;
        } else if (*s == '\t' || *s == ',' || *s == '.' || *s == '?' || *s == '!') {
            *s = ' ';
        }
    }
    for (s = ans + strlen (ans); s > ans && s[-1] == ' '; --s) {
    }
    *s = 0;
    for (s = ans; *s == ' '; ++s) {
    }
    memmove (ans, s, strlen (s) + 1);
    ++asks;
}
#endif

#if defined META_KEYS || defined META_VALUE
#ifndef META_TEXT_ON
#define META_TEXT_ON()
#define META_TEXT_OFF()
#endif
#ifndef META_LINE
#define META_LINE       lb
#endif
#endif

#ifdef META_KEYS
/* The n'th line of key (0 the first): its values in v (nv of them, their spaces trimmed); 0 none */
META_LINK unsigned char __fastcall__ words_of (const char* key, unsigned char n)
{
    unsigned p = 0, e;
    unsigned char k = strlen (key);
    char* s;

    META_TEXT_ON ();
    for (; p < twn; p = e + 1) {
        for (e = p; e < twn && tw[e] != '\n'; ++e) {
        }
        if (strncmp (tw + p, key, k) || (tw[p + k] != ' ' && tw[p + k] != '|') || n--) {
            continue;
        }
        memcpy (META_LINE, tw + p, e - p < sizeof META_LINE - 1 ? e - p : sizeof META_LINE - 1);
        META_TEXT_OFF ();
        META_LINE[e - p < sizeof META_LINE - 1 ? e - p : sizeof META_LINE - 1] = 0;
        for (nv = 0, s = strchr (META_LINE, '|'); s && nv < sizeof v / sizeof v[0]; s = strchr (s, '|')) {
            *s++ = 0;
            while (*s == ' ') {
                ++s;
            }
            v[nv++] = s;
        }
        for (k = 0; k < nv; ++k) {
            for (s = v[k] + strlen (v[k]); s > v[k] && (s[-1] == ' ' || s[-1] == '\r'); --s) {
            }
            *s = 0;
        }
        return nv;
    }
    META_TEXT_OFF ();
    return nv = 0;
}

/* The words' text read from fd into tw (its lines but its notes: i at a line's start, 2 in a note) */
META_LINK void __fastcall__ words_read (int fd)
{
    unsigned char i;
    int n, j;

    for (i = 1; (n = read (fd, META_LINE, sizeof META_LINE)) > 0; ) {
        META_TEXT_ON ();
        for (j = 0; j < n && twn < META_TEXT_MAX - 1; ++j) {
            if (i == 1 && META_LINE[j] == '#') {
                i = 2;
            }
            if (i != 2 && META_LINE[j] != '\r') {
                tw[twn++] = META_LINE[j];
            }
            i = META_LINE[j] == '\n' ? 1 : i == 2 ? 2 : 0;
        }
        META_TEXT_OFF ();
    }
}
#endif

#ifdef META_VALUE
/* Rumour k's line of key: its n'th value (the first, its lines joined) into said: 1, or 0 none.  Without META_VALUE_N, the
** first value alone: value_of (k, key) */
#ifdef META_VALUE_N
META_LINK unsigned char __fastcall__ value_of (unsigned char k, const char* key, unsigned char n)
#else
META_LINK unsigned char __fastcall__ value_of (unsigned char k, const char* key)
#endif
{
    const char* p = rbuf + rat[k];
    const char* e = rbuf + rto[k];
    unsigned char len = strlen (key), got = 0;
    unsigned m = 0;
#ifdef META_VALUE_N
    unsigned char i;
#endif

    META_TEXT_ON ();
    for (; p < e; ) {
        if (got) {
            if (*p != ' ' && *p != '\t') {
                break;
            }
            while (*p == ' ' || *p == '\t') {
                ++p;
            }
            if (m && m < sizeof said - 1) {
                said[m++] = ' ';
            }
        } else if (!strncmp (p, key, len) && (p[len] == ' ' || p[len] == '|')) {
#ifdef META_VALUE_N
            for (i = 0; i <= n && p; ++i) {
                p = strchr (p + 1, '|');
            }
#else
            p = strchr (p, '|');
#endif
            if (!p || p >= e) {
                break;
            }
            got = 1;
            ++p;
            while (*p == ' ') {
                ++p;
            }
        } else {
            for (; p < e && *p != '\n'; ++p) {
            }
            ++p;
            continue;
        }
        for (; p < e && *p != '\n' && *p != '\r' && *p != '|'; ++p) {
            if (m < sizeof said - 1) {
                said[m++] = *p;
            }
        }
#ifdef META_VALUE_N
        if (n) {
            break;                              /* (A value past the first: its line alone) */
        }
#endif
        for (; p < e && *p != '\n'; ++p) {
        }
        ++p;
    }
    while (m && said[m - 1] == ' ') {
        --m;
    }
    said[m] = 0;
    META_TEXT_OFF ();
    return got;
}
#endif

#if defined META_LAUNCH || defined META_RUN
#ifndef META_SPAWN
#define META_SPAWN      meta_spawn
#define META_JOIN       meta_join
#endif
static struct hy_regs meta_r;

/* A program started: path, its arguments (each ended by a 0, and an empty one after them): its task, or -1.  (hydra.h's
** hy_spawn, without the stdio it takes in to write its buffers out first, nor the heap its argv wants) */
META_LINK int __fastcall__ META_SPAWN (const char* path, const char* args)
{
    meta_r.a = 0;                               /* (Its fds: this one's 0, 1 and 2) */
    meta_r.r[0] = (unsigned) path;
    meta_r.r[1] = (unsigned) args;
    return hy_call (HY_SPAWN, &meta_r) ? -1 : meta_r.a;
}

/* A task waited for (hy_wait's, no message): its exit status, or -1 */
META_LINK int __fastcall__ META_JOIN (unsigned char task)
{
    meta_r.a = task;
    meta_r.r[0] = 0;
    return hy_call (HY_WAIT, &meta_r) ? -1 : meta_r.x;
}
#endif

#ifdef META_RUN
/* A program run (path, its arguments as META_SPAWN's) and waited for: its exit status, or -1 */
META_LINK int __fastcall__ meta_run (const char* path, const char* args)
{
    int t = META_SPAWN (path, args);

    return t < 0 ? -1 : META_JOIN (t);
}
#endif

#ifdef META_ARG
/* An argument at q (s, 0-ended): the next's place */
META_LINK char* __fastcall__ arg1 (char* q, const char* s)
{
    strcpy (q, s);
    return q + strlen (q) + 1;
}

/* Two: a flag and its value */
META_LINK char* __fastcall__ arg (char* q, const char* flag, const char* v)
{
    return arg1 (arg1 (q, flag), v);
}
#endif

#ifdef META_ARGN
/* A flag and a number */
META_LINK char* __fastcall__ argn (char* q, const char* flag, unsigned long v)
{
    char b[12];

    return arg (q, flag, ultoa (v, b, 10));
}
#endif
