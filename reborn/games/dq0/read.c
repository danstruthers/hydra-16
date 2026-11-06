/*
** read.c - (parts 25, 26) Dingus Quest Zero's writings read and copied, and their clues kept (the plan for the game between dungeons,
** its steps 9 and 10; HOUSE.md's reading and clues).  A writing is a theme's kind with a read rule (a plaque, a slab, a ledger,
** a pillar, a stand-in's tablet or scroll), or a thing that carries one: an old writing carried out of its own dungeon, a copy
** (items.h's KI_WRITING, KI_COPY: its clue by its fuel, life.h's).  Each has its clue (life.h's struct clue): its people's tongue
** and script (the theme's ptongue and pscript, by the era of its piece), its condition by its age (the book's, Treasure 8: an
** era's centuries, carved stone and metal kept the better), the place it names (the one-link clue's: by the theme's realms of
** today, a land and its word, the game's seed's), what it says (its rule's words).  Read only in the light, and only by one who
** knows both its tongue and its script (the referee's P_READ: the book's, Skills 11, the masteries' average and the condition),
** the four grades' words as the plan has them: a critical success the words, the place and the reason, and a line more (what
** the puzzle wants, and where); a success the words, the place and the reason; a failure a piece of the words, and the land
** alone; a critical failure a wrong reading told as confidently (the theme's own names swapped about, another place).  An
** illiterate (no script on its sheet) reads nothing; no puzzle needs reading (the theme checker's, Q9: a writing's a hint).
** Copied onto a sheet (paper or parchment, with charcoal, a pencil, or a quill and ink: the map's own things), by Drawing's
** test, the copy's quality its condition for a later reading.  The clues kept in clues.dat in the dungeon's folder (a record
** a rule: as it's read or copied, and every writing as the dungeon's left alive); dq0life carries them out (life.h's).
** Part 25's (the reading), part 26's (the clues made, and a reading's other words), both overlay 21's bank: stubs read_cmd,
** clue_make, clues_kept, more_said, swapped.  A part's data is its own and copied in afresh as it's called again (swap.inc's):
** what outlives a call to the other part is on the C stack or in the clues' files.
*/

#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "life.h"
#include "rs.h"

/* ==== Part 26: the clues made, a reading's other words */

#pragma code-name (push, "SW26CODE")
#pragma rodata-name (push, "SW26RODATA")
#pragma data-name (push, "SW26RODATA")
#pragma bss-name (push, "SW26RODATA")
#define K               K26                     /* (Its own) */
#define path            path26
static unsigned hs;                             /* (A writing's own dice: 16 bits, no long sums' code in the main RAM) */

/* A die of n (0 ... n - 1), the writing's own (hs: the game's seed and its rule, so it's the same each time) */
static unsigned char __fastcall__ die (unsigned char n)
{
    hs = hs * 25173U + 13849U;
    return (unsigned char) ((hs >> 8) % n);
}

static struct clue K;
static char path[56];

/* The dungeon's clues' file (or /ram's, without a save's folder: its records checked by the seed, seeded's), open at record n
** (-1 none) */
static int __fastcall__ here_at (unsigned char n, unsigned char wr)
{
    int fd;

    hub_map (0);
    if (HUB->savedir[0]) {
        strcpy (path, HUB->savedir);
    } else {
        strcpy (path, "/ram");
    }
    strcat (path, "/" CLUES_FILE);
    if ((fd = open (path, wr ? O_RDWR | O_CREAT : O_RDONLY, 0644)) >= 0) {
        lseek (fd, (unsigned) n * CLUE_SIZE, SEEK_SET);
    }
    return fd;
}

/* ---- A writing's clue made from the theme (rule r's): its people's tongue and script, its condition, its place */

static const char lands[6][3][10] = {           /* (Arrays, not pointers: their words in this part's data) */
    { "Peaks", "Crags", "Mountains" }, { "Hills", "Downs", "Fells" }, { "Woods", "Forests", "Wealds" },
    { "Fens", "Marshes", "Meres" }, { "Cliffs", "Shores", "Strands" }, { "Plains", "Wolds", "Heaths" }
};

/* Condition by age (the book's Written Work Condition, Treasure 8: ancient, middle, modern; d100 at most each, perfect to very
** poor, past the last rigged): its steps (+2 ... -2) */
static const unsigned char worn[3][5] = { { 15, 35, 70, 85, 95 }, { 10, 25, 50, 75, 95 }, { 2, 5, 10, 30, 95 } };

static signed char __fastcall__ worn_by (unsigned char col, unsigned char roll)
{
    unsigned char i;

    for (i = 0; i < 4 && roll > worn[col][i]; ++i) {
    }
    return 2 - i;
}

/* A place's name, the realm m's word and a land's ("the Ostmarkan Fells"), into s */
static void __fastcall__ place_of (char* s, unsigned char m, unsigned char land, unsigned char w)
{
    strcpy (s, "the ");
    if (m != 0xFF) {
        strcat (s, THEME->mint[m].adj);
        strcat (s, " ");
    }
    strcat (s, lands[land][w]);
}

/* Rule r's writing's dice begun (hs), by the game's seed: its check, its low byte (a record's carried, here) */
static unsigned char __fastcall__ seeded (unsigned char r)
{
    static unsigned long s;

    hub_map (0);
    s = HUB->seed;
    hs = (unsigned) s ^ (unsigned) (s >> 16) ^ (r + 1) * 40503U;
    return (unsigned char) hs;
}

static void __fastcall__ made (unsigned char r)
{
    static unsigned char a, k, e, ago, col, roll, nm, m, land, mat;
    static unsigned char cand[TMINTS];
    static signed char app;

    memset (&K, 0, sizeof K);
    K.carried = seeded (r);                     /* (Its check: a record of another game's made again) */
    hub_map (THEME_BANK);
    a = THEME->rule[r].a;
    K.state = CL_UNREAD;
    K.rule = r;
    K.copy = CLUE_NOCOPY;
    strcpy (K.thing, THEME->kind[a].name);
    mat = THEME->kind[a].mat;
    for (e = k = 0; k < THEME->npieces && k < NPIECES; ++k) {
        if (THEME->piece[k].kind == a) {
            e = THEME->pera[k];                 /* (The era it lies among: its people's) */
            break;
        }
    }
    if (!e && THEME->neras) {
        e = 1;                                  /* (Its history's first people's, none said) */
    }
    ago = 0;
    K.work = W_KEEP;
    if (e) {
        k = THEME->era[e - 1].people;
        strcpy (K.tongue, THEME->ptongue[k]);
        strcpy (K.script, THEME->pscript[k]);
        K.work = THEME->era[e - 1].work;
        ago = THEME->era[e - 1].ago;
    }
    strncpy (K.words, (const char*) THEME + THEME->rule[r].words, sizeof K.words - 1);
    for (nm = k = 0; k < THEME->nmints && k < TMINTS; ++k) {
        if (THEME->mint[k].era == e && e) {
            strcpy (K.people, THEME->mint[k].name);     /* (Who wrote it: its era's own coins' name, "the hill-kings") */
        }
        if (!THEME->mint[k].era && !(THEME->mint[k].ways & MW_LOCAL)) {
            cand[nm++] = k;                     /* (The realms about, not the region's own: where it points) */
        }
    }
    K.ago = ago;                                /* (Its age: the scholar's, a dead tongue's) */
    col = ago < 3 ? 0 : ago < 10 ? 1 : 2;       /* (Modern, middle, ancient: centuries; carved stone and metal the younger) */
    if (col && (mat == MT_STONE || mat == MT_IRON || mat == MT_BRONZE || mat == MT_STEEL || mat == MT_SILVER || mat == MT_GOLD)) {
        --col;
    }
    roll = die (100) + 1;
    app = worn_by (col, roll > 95 ? die (95) + 1 : roll);
    K.cond = roll > 95 ? app - 4 : app;         /* (Rigged: misleading, its apparent condition told, 20 the worse) */
    land = die (6);
    m = nm ? cand[die (nm)] : 0xFF;
    place_of (K.place, m, land, die (3));
    K.land = land | (app + 2) << 4;
    land = (land + 1 + die (5)) % 6;            /* (A wrong reading's: another land, another realm if there is one) */
    if (nm > 1) {
        for (k = 0; cand[k] != m; ++k) {
        }
        m = cand[(k + 1) % nm];
    }
    place_of (K.wrong, m, land, die (3));
    K.work |= land << 4;                        /* (The wrong place's land: dq0life's rumour of it) */
    hub_map (0);
}

/* Rule r's clue in the dungeon's file, made there if it isn't */
void __fastcall__ s_clue_make (unsigned char r)
{
    int fd;
    unsigned char ok = 0;

    if ((fd = here_at (r, 0)) >= 0) {
        ok = read (fd, &K, CLUE_SIZE) == CLUE_SIZE && K.state && K.carried == seeded (r);
        close (fd);
    }
    if (!ok && (fd = here_at (r, 1)) >= 0) {
        made (r);
        write (fd, &K, CLUE_SIZE);
        close (fd);
    }
}

/* The dungeon left alive (theme.c's ending): every writing of its theme's kept in its clues, those not read yet as unread (none
** without a save's folder) */
#pragma static-locals (push, off)
void s_clues_kept (void)
{
    unsigned char r;
    unsigned w = hub_here ();                   /* (Called from overlay 15: its window put back) */

    hub_map (0);
    if (HUB->savedir[0]) {
        for (r = 0; r < NRULES; ++r) {
            hub_map (THEME_BANK);
            if (r < THEME->nrules && THEME->rule[r].verb == TR_READ) {
                s_clue_make (r);
            }
        }
    }
    hub_back (w);
}
#pragma static-locals (pop)

/* A critical success's line more (here alone): the thing the puzzle wants, a rule's that solves it, and where it lies if
** that's been decided (the maker's place for its piece) */
void s_more_said (void)
{
    static unsigned char r, a, k, sn;

    hub_map (THEME_BANK);
    for (r = 0; r < THEME->nrules && r < NRULES && !THEME->rule[r].effect; ++r) {
    }
    if (r < THEME->nrules && r < NRULES) {
        a = THEME->rule[r].a;
        strcpy (tname, THEME->kind[a].name);
        sn = 0xFF;
        for (k = 0; k < THEME->npieces && k < NPIECES; ++k) {
            if (THEME->piece[k].kind == a) {
                sn = THEME_PLACE[k].sn;
            }
        }
        hub_map (0);
        sayf ("  Under it, in a smaller hand, a line most would miss: the %s is what matters here", tname);
        if (sn < TSTRATA) {
            hub_map (THEME_BANK);
            strcpy (tname, THEME->stratum[sn]);
            hub_map (0);
            sayf (", and it lies in %s", tname);
        }
        say (".");
    }
    hub_map (0);
}

/* A critical failure's words: rule r's writing's (here alone), the theme's own things' names swapped about, each by its last
** word as a word ("seal" for "crown", "crown" for "throne": the king's seal, the iron crown, the throne), into lb */
static char names[NTKINDS][14];

static unsigned char __fastcall__ is_letter (char c)
{
    return (c | 0x20) >= 'a' && (c | 0x20) <= 'z';
}

void __fastcall__ s_swapped (unsigned char r)
{
    int fd;
    static unsigned char k, j, n, i;
    static unsigned o;                          /* (lb's 400: past a byte's count) */
    static const char* p;
    static char* w;

    lb[0] = 0;
    if ((fd = here_at (r, 0)) < 0) {
        return;
    }
    read (fd, &K, CLUE_SIZE);
    close (fd);
    hub_map (THEME_BANK);
    for (k = 0; k < NTKINDS; ++k) {
        w = strrchr (THEME->kind[k].name, ' ');
        strcpy (names[k], w ? w + 1 : THEME->kind[k].name);
    }
    hub_map (0);
    for (o = 0, p = K.words; *p && o < sizeof lb - 16; ) {
        for (k = 0; k < NTKINDS; ++k) {
            n = strlen (names[k]);
            if (n > 2 && (p == K.words || !is_letter (p[-1])) && !strncmp (p, names[k], n) && !is_letter (p[n])) {
                for (j = k, i = 0; i < NTKINDS; ++i) {      /* (The next name not the same, if there is one) */
                    j = (j + 1) % NTKINDS;
                    if (names[j][0] && strcmp (names[j], names[k])) {
                        break;
                    }
                }
                strcpy (lb + o, names[j]);
                o += strlen (names[j]);
                p += n;
                break;
            }
        }
        if (k == NTKINDS) {
            lb[o++] = *p++;
        }
    }
    lb[o] = 0;
}

#undef K
#undef path
#pragma bss-name (pop)
#pragma data-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* ==== Part 25: the reading */

#pragma code-name (push, "SW25CODE")
#pragma rodata-name (push, "SW25RODATA")
#pragma data-name (push, "SW25RODATA")
#pragma bss-name (push, "SW25RODATA")

static struct clue K;                           /* The clue at hand */
static char path[56];

/* The clues' file: the dungeon's (here), or the save's top (its life's), open at record n; -1 none */
static int __fastcall__ clues_at (unsigned char top, unsigned n, unsigned char wr)
{
    int fd;

    hub_map (0);
    if (top) {
        if (!savedir) {
            return -1;
        }
        strcpy (path, savedir);
    } else if (HUB->savedir[0]) {
        strcpy (path, HUB->savedir);
    } else {
        strcpy (path, "/ram");
    }
    strcat (path, "/" CLUES_FILE);
    if ((fd = open (path, wr ? O_RDWR | O_CREAT : O_RDONLY, 0644)) >= 0) {
        lseek (fd, (unsigned) n * CLUE_SIZE, SEEK_SET);
    }
    return fd;
}

/* Clue id (a thing's fuel: CLUE_HERE | its rule, else its number at the top) read into K (0 none), or written */
static unsigned char __fastcall__ clue_io (unsigned id, unsigned char wr)
{
    int fd = clues_at (!(id & CLUE_HERE), id & CLUE_HERE ? id & 0xFF : (id & ~CLUE_COPY) - 1, wr);
    unsigned char ok = 0;

    if (fd >= 0) {
        ok = wr ? write (fd, &K, CLUE_SIZE) == CLUE_SIZE : read (fd, &K, CLUE_SIZE) == CLUE_SIZE && K.state;
        close (fd);
    }
    return ok;
}

/* ---- Reading */

static const char wears[5][52] = {             /* (Arrays, not pointers: their words in this part's data) */
    "The marks are worn almost away.", "The marks are worn.", "", "The marks are clear.",
    "The marks are as crisp as the day they were made."
};
static const char works[6][28] = {
    "raised another keep", "built another temple", "dug other mines", "laid others of their dead", "built another town",
    "carved another hold"
};
static const char lies[6][18] = { "in the mountains", "in the hills", "in the woods", "in the marshes", "on the coast",
    "on the plains" };

/* Its place and its reason told (the place a wrong reading's, wr) */
static void __fastcall__ place_said (unsigned char wr)
{
    if (K.place[0]) {
        sayf ("  It tells of %s, where %s %s.", wr ? K.wrong : K.place, K.people[0] ? K.people : "its makers",
            works[((K.work & 15) + wr) % 6]);
    }
}

/* Clue id (K) read, at the steps cond (its apparent condition app's words first): the referee's test, the grade's words, the
** clue kept as it's been read.  Its locals on the C stack: part 26's words called (a critical success's, a critical failure's) */
#pragma static-locals (push, off)
static void __fastcall__ read_it (unsigned id, signed char cond, signed char app)
{
    struct req* q;
    signed char g;
    unsigned char res, n, here = (id & (CLUE_HERE | CLUE_COPY)) == CLUE_HERE;

    if (app > -3 && app < 3 && wears[app + 2][0]) {
        msg (wears[app + 2]);
    }
    n = K.state & ~CL_MORE;
    if (n > CL_UNREAD) {                        /* (Read before: what was read, the same again, untested) */
        g = n == CL_PIECE ? G_MF : n == CL_WRONG ? G_CF : K.state & CL_MORE ? G_CS : G_MS;
        goto told;
    }
    hub_map (0);
    q = &HUB->slot[SLOT_UI];
    q->kind = RS_TEST;
    q->actor = 0;
    q->a = P_READ;
    q->b = 0;
    q->steps = cond;
    q->w1 = id;
    strcpy (q->text, K.tongue[0] ? K.tongue : "common");
    strcat (q->text, " *");
    strcat (q->text, K.script[0] ? K.script : "ardic");
    q = hub_ask (SLOT_UI);
    g = q->grade;
    res = q->result;
    q->state = REQ_FREE;
    if (res) {
        sayf (res == 1 ? "\nYou can't read: the marks on the %s mean nothing to you." : res == 2 ?
            "\nYou know the letters on the %s, but the words they spell are in a tongue you don't speak." :
            "\nThe %s is written in letters you've never learnt.", K.thing);
        intend2 (I_WAIT, 100, 1);
        return;
    }
    K.state = g == G_MF ? CL_PIECE : g == G_CF ? CL_WRONG : g == G_CS ? CL_RIGHT | CL_MORE : CL_RIGHT;
    clue_io (id, 1);                            /* (Kept first: part 26 called, this part's K is its image's again) */
told:
    if (g == G_MF) {                            /* (A piece of it: its first half, to a word's end) */
        strcpy (lb, K.words);
        for (n = strlen (lb) / 2; n && lb[n] != ' '; --n) {
        }
        strcpy (lb + n, " ...");
        msg (lb);
        sayf ("  The rest you can't make out, but it speaks of some place %s.", lies[(K.land & 15) % 6]);
    } else {
        if (g == G_CF && here) {
            swapped (id);                       /* (Into lb, its record read there; K again after it) */
            clue_io (id, 0);
            msg (lb);
        } else {
            msg (K.words);
        }
        place_said (g == G_CF);                 /* (A wrong reading told as confidently as a right one) */
        if (g == G_CS && here) {
            more_said ();
        }
    }
    intend2 (I_WAIT, 600, 1);                   /* (A minute's reading) */
}
#pragma static-locals (pop)

/* A writing read or copied (c's: VB_READ_OBJECT, its verb's skill Drawing's a copy): Q_TOLD; c 0, the dungeon left alive, every
** writing of its theme's kept in its clues (those not read yet as unread): 0 */
static unsigned char __fastcall__ reading (struct cmd* c)
{
    static unsigned char copy, r, k;
    static unsigned x;
    static signed char g;

    copy = c->skill == P_DRAW + 1;
    if (!c->obj.word[0]) {
        msg (c->obj.kind ? "There's nothing written on it." : copy ? "Copy what?" : "Read what?");
        return Q_TOLD;
    }
    hub_map (SIGHT_BANK);
    k = SIGHT->dark;
    hub_map (0);
    if (k) {
        msg (copy ? "You can't see to copy anything." : "You can't see to read.");    /* (In the dark, or blind) */
        return Q_TOLD;
    }
    walk_op = IO_PUT;                           /* (Its first words before this asking: walk_ahead's, sway.c) */
    walk_ahead (c->obj.word);
    if (othings (IO_LOOK, c->obj.word, "", LM_COND) == IR_NOT_HERE) {
        reach_go ();                            /* (Out of reach: walked over to first, then again, as any inspecting) */
        return Q_TOLD;
    }
    r = othings (IO_PUZZLE, c->obj.word, "", copy ? TR_COPY : TR_READ);
    x = othings_x;
    copy = c->skill == P_DRAW + 1;              /* (Again: a part called meanwhile copies this one in afresh, swap.inc's) */
    if (lb[0]) {
        msg (lb);                               /* (The things' words: none to hand) */
        return Q_TOLD;
    }
    if (r == PZ_NONE) {
        sayf (copy ? "\nThere's nothing written on the %s to copy." : "\nThere's nothing written on the %s.", c->obj.word);
        return Q_TOLD;
    }
    if (copy) {
        if (r == PZ_NO) {
            msg ("You have nothing to copy it onto: a sheet, and charcoal, a pencil, or a quill and ink.");
            return Q_TOLD;
        }
        clue_make (x);                          /* (Made if it isn't: part 26's; this part's statics afresh after it) */
        x = othings_x;
        clue_io (CLUE_HERE | x, 0);
        g = ask_b (RS_TEST, P_DRAW, 0)->grade;  /* (The book's Drawing, its Cartography first: the referee's) */
        HUB->slot[SLOT_UI].state = REQ_FREE;
        K.copy = g == G_CS ? 2 : g == G_MS ? 1 : g == G_MF ? -1 : -2;
        sayf ("\nYou copy the marks of the %s onto your sheet, line by line: %s.", c->obj.word, g == G_CS ?
            "a fair copy, every mark as it stands" : g == G_MS ? "a good copy" : g == G_MF ? "a rough copy, some of its marks guessed at" :
            "a poor copy, much of it botched");
        clue_io (CLUE_HERE | x, 1);
        intend2 (I_WAIT, 6000, 1);              /* (Ten minutes' copying) */
        return Q_TOLD;
    }
    if (r != PZ_DONE && r != PZ_OLD) {
        return Q_TOLD;
    }
    if (r == PZ_OLD) {                          /* (An old writing, a copy: its clue by its fuel) */
        if (!clue_io (x, 0)) {
            if (!(x & CLUE_HERE)) {
                sayf ("\nThe %s's marks have run past reading.", c->obj.word);   /* (Its record gone) */
                return Q_TOLD;
            }
            clue_make (x & 0xFF);
            x = othings_x;
            clue_io (x, 0);
        }
        if (x & CLUE_COPY) {
            g = K.copy == CLUE_NOCOPY ? 0 : K.copy;
            read_it (x, g, g);
        } else {
            read_it (x, K.cond, (K.land >> 4) - 2);
        }
        return Q_TOLD;
    }
    clue_make (x);                              /* (A writing of this dungeon's, its rule x) */
    x = othings_x;
    clue_io (CLUE_HERE | x, 0);
    read_it (CLUE_HERE | x, K.cond, (K.land >> 4) - 2);
    return Q_TOLD;
}

/* (Called from overlay 5: the window put back as it was, the hub's header in it meanwhile, as othings does) */
#pragma static-locals (push, off)
unsigned char __fastcall__ s_read_cmd (struct cmd* c)
{
    unsigned w = hub_here ();
    unsigned char q = reading (c);

    hub_back (w);
    return q;
}
#pragma static-locals (pop)

#pragma bss-name (pop)
#pragma data-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
