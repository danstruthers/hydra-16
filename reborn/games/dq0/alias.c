/*
** alias.c - (part 9) the player's shortcuts (the user, 2026-10-22: "alias should be for shortcuts, and name for naming
** things"; the command grammar's alias-cmd, commandcompile.js's alias): "alias x is dig down" (or "alias x = dig
** down") a word standing for a line, asked before one's replaced; "alias x" one shown; "alias" them all, and the
** game's own words (its lib folder's aliases.txt: commands.aliases, the parser's); "unalias x" (or "forget alias x")
** one removed.  A word of the commands' ("look", "take") can't be one; "alias the sword as Linda" is sent to name.  A
** line typed whose first word is one (term.c's enter, as it's entered, before its parts: so a shortcut may hold
** several commands, "north then dig down") is made its line, the rest typed after it, once: a shortcut in its line not
** made again (but one whose line begins with another's is given that one's line as it's made, as the reference's).  A
** word of the game's own ("peer") may be made a shortcut: the player's then, as a line's first word; the game's own
** words can't be removed (HOUSE.md's commands).  The shortcuts are kept in the theme bank (hub.h's ALIAS_AT: TALIASES,
** and one more for a replacement asked about) and in the save.  Its commands, its making a line and its keeping are
** a swapped part's (part 9, a bank of its own: swap.inc), called through bank.s's stubs; a line's first look the
** main code's (alias_hit: a count and a hash of the shortcuts' words) and overlay 4's (alias_said)
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"
#include "hub.h"

#pragma local-strings (on)                      /* (A function's strings beside it, in the part) */

unsigned char aliases_n;                        /* The shortcuts made (0: none: a line's first word not looked up) */
unsigned char alias_wait;                       /* A replacement asked about: the next line its answer */
static unsigned char alias_bits[8];             /* The shortcuts' words, hashed (names_hash's, 64 bits of it) */

/* A line's first word maybe a shortcut's (alias_bits: the part makes sure) */
unsigned char __fastcall__ alias_hit (const char* l)
{
    static unsigned char h;

    h = names_hash (l);
    return alias_bits[h >> 3 & 7] & 1 << (h & 7);
}

#pragma code-name (push, "OV4CODE")             /* ---- Overlay 4: a line's first look */
#pragma rodata-name (push, "OV4RODATA")

/* A line the shortcuts' own: "alias ...", "unalias ...", "forget alias ..." (its start: the part makes sure) */
unsigned char __fastcall__ o_alias_said (const char* l)
{
    return !strnicmp (l, "alias", 5) || !strnicmp (l, "unalias", 7) || !strnicmp (l, "forget alias", 12);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW9CODE")             /* ---- Part 9: the commands, a line made, the save's */
#pragma rodata-name (push, "SW9RODATA")

#define a           (*(struct talias*) (lb + 256))  /* An entry, copied out (lb's end: its start the line's, own's) */
#define w           (lb + 368)                  /* A word of the line's (its case aside, 15 letters at most), */
#define root        (lb + 384)                  /*   and a game's own word's meaning (own's) */
#define WMAX        15

/* Entry i copied out into a, or back */
static void __fastcall__ get (unsigned char i)
{
    hub_map (THEME_BANK);
    memcpy (&a, ALIAS_AT + i, sizeof a);
    hub_map (0);
}

static void __fastcall__ put (unsigned char i)
{
    hub_map (THEME_BANK);
    memcpy (ALIAS_AT + i, &a, sizeof a);
    hub_map (0);
}

/* The entry whose word is k (the main RAM's: the bank in the window meanwhile): its number, or 0xFF; with k "", a
** free one */
static unsigned char __fastcall__ find (const char* k)
{
    unsigned char i;

    hub_map (THEME_BANK);
    for (i = 0; i < TALIASES && stricmp (ALIAS_AT[i].word, k); ++i) {
    }
    hub_map (0);
    return i < TALIASES ? i : 0xFF;
}

/* The shortcuts counted (aliases_n), their words hashed (alias_bits) */
static void count (void)
{
    unsigned char i, h;

    aliases_n = 0;
    memset (alias_bits, 0, sizeof alias_bits);
    hub_map (THEME_BANK);
    for (i = 0; i < TALIASES; ++i) {
        if (ALIAS_AT[i].word[0]) {
            ++aliases_n;
            h = names_hash (ALIAS_AT[i].word);
            alias_bits[h >> 3 & 7] |= 1 << (h & 7);
        }
    }
    hub_map (0);
}

/* The line's next word into w (lower case, WMAX letters at most): what follows it, its spaces passed */
static const char* __fastcall__ next (const char* l)
{
    unsigned char n;

    while (*l == ' ') {
        ++l;
    }
    for (n = 0; *l && *l != ' '; ++l) {
        if (n < WMAX) {
            w[n++] = *l >= 'A' && *l <= 'Z' ? *l + ('a' - 'A') : *l;
        }
    }
    w[n] = 0;
    while (*l == ' ') {
        ++l;
    }
    return l;
}

/* The game's own words (aliases.txt, beside the overlays: "word root" a line, # a comment): k's meaning into root
** (1; 0: not one of them); or, with k 0, them all said */
static unsigned char __fastcall__ own (const char* k)
{
    int fd, n;
    unsigned char j, i, found;
    char* p;
    char* q;

    strcpy (lb, libdir);
    strcat (lb, "/aliases.txt");
    if ((fd = open (lb, O_RDONLY)) < 0) {
        return 0;
    }
    for (j = found = 0;;) {                     /* (A line at a time in lb, 200 bytes at most) */
        n = read (fd, lb + j, 200 - j);
        if (n > 0) {
            j += n;
        }
        for (i = 0; i < j && lb[i] != '\n'; ++i) {
        }
        if (i == j && n > 0 && j < 200) {
            continue;
        }
        if (!j) {
            break;
        }
        lb[i] = 0;
        for (n = i; n && (lb[n - 1] == '\r' || lb[n - 1] == ' '); ) {
            lb[--n] = 0;
        }
        q = lb + (*lb == '*');                  /* (Its word or words, "look closely", a * before an anywhere one's;
                                                **   the root the last word: commandcompile.js's loadAliases) */
        if (*lb && *lb != '#' && (p = strrchr (q, ' ')) != 0) {
            *p++ = 0;
            if (!k) {
                sayf (found++ ? ", %s (%s)" : " %s (%s)", q, p);
            } else if (!stricmp (q, k)) {
                p[WMAX] = 0;
                strcpy (root, p);
                found = 1;
                break;
            }
        }
        i += i < j;
        memmove (lb, lb + i, j - i);
        j -= i;
    }
    close (fd);
    return found != 0;
}

/* Every shortcut said, then the game's own words */
static void list (void)
{
    unsigned char i, n;

    for (i = n = 0; i < TALIASES; ++i) {
        get (i);
        if (a.word[0]) {
            sayf (n++ ? "; %s: %s" : "\nYour shortcuts: %s: %s", a.word, a.line);
        }
    }
    if (n) {
        say (".");
    } else {
        msg ("You've made no shortcuts (alias x is dig down makes one).");
    }
    msg ("The game's own words:");
    own (0);
    say (".");
}

/* Line r's words after the shortcut's word into lb (from 0), as the shortcut's line: its first word a shortcut's,
** that one's line, as it is now, the rest after it (each INMAX at most) */
static void __fastcall__ line_of (const char* r)
{
    const char* s;
    unsigned char i;

    s = next (r);
    if (w[0] && stricmp (w, a.word) && (i = find (w)) != 0xFF) {
        hub_map (THEME_BANK);
        strcpy (lb, ALIAS_AT[i].line);
        hub_map (0);
        if (*s) {
            i = strlen (lb);
            lb[i++] = ' ';
            strncpy (lb + i, s, INMAX);
            lb[i + INMAX] = 0;
        }
    } else {
        strncpy (lb, r, INMAX);
        lb[INMAX] = 0;
    }
}

/* A line the shortcuts' own (main.c's, the line's first look alias_said's; or any line, as a replacement's asked
** about): "alias x is dig down", "alias x", "alias", "unalias x", "forget alias x"; a replacement's yes or no.  1: done
** here; 0: the line's another's (the command's) */
unsigned char __fastcall__ s_alias_line (char* l)
{
    const char* r;
    unsigned char i, un;

    if (alias_wait) {                           /* (A replacement asked about: this line its answer, or another
                                                **   command, the replacement dropped) */
        alias_wait = 0;
        next (l);
        if (!strcmp (w, "y") || !strcmp (w, "yes")) {
            get (TALIASES);
            put (find (a.word));
            sayf ("\n%s now stands for: %s", a.word, a.line);
            count ();
            return 1;
        }
        if (!strcmp (w, "n") || !strcmp (w, "no")) {
            msg ("It stands as it was.");
            return 1;
        }
    }
    r = next (l);
    un = !strcmp (w, "unalias");
    if (!strcmp (w, "forget")) {
        r = next (r);
        if (strcmp (w, "alias")) {
            return 0;
        }
        un = 1;
    } else if (!un && strcmp (w, "alias")) {
        return 0;                               /* ("aliases", "alibi": not this) */
    }
    r = next (r);                               /* (The shortcut's word, w; r what follows it) */
    if (un) {
        if (!w[0]) {
            msg ("Unalias which?");
        } else if ((i = find (w)) != 0xFF) {
            a.word[0] = 0;
            put (i);
            count ();
            sayf ("\n%s isn't a shortcut now.", w);
        } else if (own (w)) {
            sayf ("\n%s is the game's own word (for %s): it can't be removed, but alias %s is ... makes it yours.",
                w, root, w);
        } else {
            sayf ("\n%s isn't a shortcut.", w);
        }
        return 1;
    }
    if (!w[0]) {
        list ();
        return 1;
    }
    if (!*r) {                                  /* (One shown) */
        if ((i = find (w)) != 0xFF) {
            get (i);
            sayf ("\n%s stands for: %s", a.word, a.line);
        } else if (own (w)) {
            sayf ("\n%s is the game's own word for %s.", w, root);
        } else {
            sayf ("\n%s isn't a shortcut.", w);
        }
        return 1;
    }
    if (*r == '=') {                            /* ("alias x = dig down", "alias x is dig down") */
        for (++r; *r == ' '; ++r) {
        }
    } else if ((r[0] | 0x20) == 'i' && (r[1] | 0x20) == 's' && r[2] == ' ') {
        for (r += 3; *r == ' '; ++r) {
        }
    } else {
        msg (strstr (l, " as ") ? "To name a thing, use name: name the sword Linda." : "Say it as: alias x is dig down.");
        return 1;
    }
    if (!*r) {
        msg ("Alias it as what?");
        return 1;
    }
    if (!strcmp (w, "alias") || !strcmp (w, "unalias") || !strcmp (w, "forget") || strchr (w, '#')) {
        msg ("That word is the shortcuts' own: choose another.");
        return 1;
    }
    if (alias_said (r)) {
        msg ("A shortcut can't make another.");
        return 1;
    }
    strcpy (a.word, w);
    if (find (w) == 0xFF && !own (w)) {         /* (A word of the commands', not the game's own shortcut: refused,
                                                **   as a name is, names.c's taken) */
        cq_drop ();                             /* (The line's, posted as it was typed, let go) */
        cq_cmd (a.word);
        cq_drop ();
        if (!(cmd.flags & CF_NOVERB) || cmd.dir != 0xFF || cmd.obj.kind || cmd.obj.sel || cmd.n ||
            stricmp (cmd.obj.word, a.word)) {
            sayf ("\n%s is one of the game's own command words: choose another.", a.word);
            return 1;
        }
    }
    line_of (r);
    if (strlen (lb) >= sizeof a.line) {
        sayf ("\nThat's too long for a shortcut (%u letters at most).", sizeof a.line - 1);
        return 1;
    }
    strcpy (a.line, lb);
    if ((i = find (a.word)) != 0xFF) {          /* (Its word a shortcut already: asked first) */
        put (TALIASES);
        get (i);
        sayf ("\n%s stands for: %s.  Replace it? (yes or no)", a.word, a.line);
        alias_wait = 1;
        return 1;
    }
    if ((i = find ("")) == 0xFF) {
        sayf ("\nYou have all the shortcuts you can (%u): unalias one first.", TALIASES);
        return 1;
    }
    put (i);
    count ();
    sayf ("\n%s now stands for: %s", a.word, a.line);
    return 1;
}

/* The line typed (inl: term.c's enter, its first word a shortcut's maybe, alias_hit's): made its shortcut's line, the
** rest typed after it (the line's length at most) */
void s_alias_expand (void)
{
    const char* r;
    unsigned char i, n;

    r = next (inl);
    if (!w[0] || (i = find (w)) == 0xFF) {
        return;
    }
    get (i);
    strcpy (lb, a.line);
    if (*r && (n = strlen (lb)) < INMAX) {
        lb[n++] = ' ';
        strncpy (lb + n, r, INMAX - n);
    }
    lb[INMAX] = 0;
    strcpy (inl, lb);
}

/* The shortcuts kept: written to the save's fd (back 0), read back from it (1; fd -1: none, a new game's), an entry
** at a time through the main RAM; the one asked about not kept */
void __fastcall__ s_alias_keep (int fd, unsigned char back)
{
    unsigned char i;

    alias_wait = 0;
    for (i = 0; i < TALIASES; ++i) {
        if (back) {
            if (fd < 0 || read (fd, &a, sizeof a) != sizeof a) {
                memset (&a, 0, sizeof a);
            }
            put (i);
        } else {
            get (i);
            write (fd, &a, sizeof a);
        }
    }
    count ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)
