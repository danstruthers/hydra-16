/*
** blows.c - (overlay 6) a fight's blow told (an E_FIGHT: the character's at a denizen, or one's at it): its words
** said.txt's FIGHT rows, the UI's own kind (UK_FIGHT; fight_words' and grip_words' before, 2026-11-12: narration as
** data, stage 4), "You strike at the giant rat with your sword: a serious cut to its left thigh (S2). The giant rat
** falls senseless."  Here what's the blow's own: its record read (copied into lb's last FIGHT_LB bytes: the words never
** as long as the rest), who's seen, the referee's four lines cut apart and told what they are (a wound, the armour that
** stopped it), the attacker's weapon's word.  And the stance, a blow's target, a grapple's.
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <string.h>
#include "hub.h"
#include "rs.h"
#include "dq0.h"
#include "cmds.h"
#include "dq0said.h"

unsigned char begun, begun_n, said_skip;      /* (dq0.h's: the character's blow begun, its first words' length, and
                                                **   those not to be said again: fight_begin's, lock2's F1) */

/* A creature's body (hub.h's BD_*, the actor's: kinds.txt's), for its getting up in a fight (the book's Rise: said.txt's
** @rises, its fall told after the blow, the world's E_BEAT WB_FALLEN) */
static unsigned char __fastcall__ body_of (unsigned char k)
{
    unsigned char b;

    hub_mem (0, &b, &HUB->actor[(k - 1) & 7].body, 1);
    return b < NBODIES ? b : BD_TWO;
}

/* Actor k - 1 a person (the hub's folk: one of the speaking peoples, said "they", the user's, 2026-11-17); k 0, the
** character: 0 */
static unsigned char __fastcall__ folk_of (unsigned char k)
{
    unsigned char f = 0;

    if (k) {
        hub_mem (0, &f, &HUB->folk[(k - 1) & 7], 1);
    }
    return f;
}

/* A strike's line told (UK_FIGHT's s2, s3): 1 a wound ("a serious cut ..."), 2 the armour that stopped it ("your leather
** tunic", "its ..."), 0 none (no harm) */
static unsigned char __fastcall__ struck (const char* l)
{
    return *l == 'a' && l[1] == ' ' ? 1 : *l ? 2 : 0;
}

/* A strike's line of one not seen, "something": its "their" (the referee's, a person's) made "its" ("a serious gash to
** its neck": the user's people "they", 2026-11-17, not a word of what can't be seen) */
static void __fastcall__ unthey (char* l)
{
    if ((l = strstr (l, "their ")) != 0) {
        memcpy (l, "its", 3);
        memmove (l + 3, l + 5, strlen (l + 5) + 1);
    }
}

/* A blow (an E_FIGHT: the character's, or one at it), worded; the character dead of it, the game's end; knocked
** senseless, busy till it comes round (the world's: its E_READY then, the hub's pc_out till it does; the UI's wait for it
** before 2026-11-13, taken at the clock it came to the world, the black-out's end with it), no line taken meanwhile
** (main.c's out_line).  Senseless (out_told, till it comes round: main.c's numb): no blow felt nor heard, a blow at it
** or between others (the user's, 2026-11-14: a knocked-out character senses nothing; its wounds there as it comes
** round), but its death: "You are dead.", the game's end; struck senseless again as it lies, out the longer */
unsigned __fastcall__ o_fight_said (const struct msg* m);
void __fastcall__ o_blow (const struct msg* m)
{
    unsigned out = out_told ? (E_FIGHT_FF (*m) & FF_D_DOWN) && !E_FIGHT_DEF (*m) : o_fight_said (m);
    unsigned char c;

    hub_mem (0, &c, (const void*) &HUB->pcsum.cond, 1);
    if (c & C_DEAD) {
        if (out_told) {
            msg ("You are dead.");              /* (Dead as it lay: the blow unfelt, its end told) */
        }
        ending (END_DIED);                         /* (The game's end, as harm's is) */
    } else if (out) {
        hub_mem (0, &c, (const void*) &HUB->pc_out, 1);
        if (c) {
            out_told |= 1;                      /* (Out still, as the world has it: free as it comes round, E_READY;
                                                **   nothing sensed till then, main.c's numb) */
            hub_mem (0, &out_until, (const void*) &HUB->pc_ready, 4);
            busy = 1;
            t_prompt (0);
        }
    }
}

/* Whether the character sees one named s (who: its actor plus 1, 0 a stray's): told seen as it closed with it, the
** senses' look not since, by its name as it was told then (2026-11-24); else among those the senses' last look saw
** (a step behind it), or one of its name: 1, or 0.  Overlay 6's; overlay 15's fight_begin through its stub, fight_seen */
#pragma static-locals (push, off)               /* (On the C stack, the main RAM: hub_mem's copies) */
unsigned char __fastcall__ o_fight_seen (const char* s, unsigned char who)
{
    char at[12];
    unsigned char k, n;

    hub_mem (SIGHT_BANK, &k, &SIGHT->seq, 1);
    if (k == close_seq && close_who && close_who == who) {
        return 1;
    }
    hub_mem (SIGHT_BANK, &n, &SIGHT->ndens, 1);
    for (k = n; k--; ) {
        hub_mem (SIGHT_BANK, at, SIGHT->den[k].name, sizeof at);
        if (!strcmp (at, s)) {
            return 1;
        }
    }
    return 0;
}
#pragma static-locals (pop)

/* The character's blow begun (o_fore_sync's: a line in hand held behind its round, the world's slot its RS_ATTACK at an
** actor posted, every event told): its first words said before the referee works the blow out, "You strike at the giant
** rat", the rest of the sentence its E_FIGHT's (o_fight_said's: begun, said_skip).  Only a melee blow struck at one
** standing (not a shot, a grapple, a kick or a butt, not from the floor, not at one lying senseless): its words begin
** so whatever comes of it (said.txt's FIGHT.begun, @blow's first words).  The first letter of a line held behind a
** fight's round no longer waits for the referee's RS_ATTACK, some 150,000-250,000 cycles (lock2's F1, 2026-11-25).
** Overlay 15's (overlay 6 full) */
#pragma code-name (push, "OV15CODE")
#pragma rodata-name (push, "OV15RODATA")
#pragma local-strings (push, on)
#pragma static-locals (push, off)
void o_fight_begin (void)
{
    struct { unsigned char other, steps, a, b, c; unsigned w1, w2; } q;
    char nm[13];
    struct msg c;
    unsigned char k, d, u;

    hub_mem (0, &q, &HUB->slot[SLOT_WORLD].other, sizeof q);
    hub_mem (0, &k, &HUB->pc_strike, 1);
    if ((d = q.other) == 0 || d > NACTORS || (q.w2 & (RS_MISSILE | RS_GRAPPLE | RS_PRONE_A)) || k == SK_KICK || k == SK_HEAD) {
        return;
    }
    hub_mem (0, &k, &HUB->actor[d - 1].doing, 1);
    if (k == DO_DOWN) {
        return;
    }
    hub_mem (0, nm, HUB->actor[d - 1].name, 12);
    nm[12] = 0;
    u = !fight_seen (nm, d);
    hub_mem (0, &k, &HUB->pc_aim, 1);
    said_s[0] = u ? "?" : nm;
    said_s[3] = 0;
    c.kind = UK_FIGHT;
    c.who = 0;
    c.a = 0;
    c.b = k & FW_FIRST;
    c.w1 = SAID_FIGHT_BEGUN << 8;
    hub_mem (0, &k, &HUB->folk[d - 1], 1);
    c.w2 = u ? 32 : k << 7;
    said_q = 1;
    outcome_words (&c);
    said_q = 0;
    begun_n = strlen (lb);
    say (lb);
    t_flush ();
    begun = d | (u ? 0x80 : 0);
}
#pragma static-locals (pop)
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* Blow m told (UK_FIGHT, said.txt's FIGHT rows): the tenths the character's knocked senseless for, or 0 */
#pragma static-locals (push, off)               /* (Its event on the C stack, the main RAM: the words' overlay reads it) */
unsigned __fastcall__ o_fight_said (const struct msg* m)
{
    unsigned char b;
    static const char* line[4];                 /* (The referee's lines: the attacker's weapon, the defender's, what the
                                                **   attacker's strike did, what the defender's did) */
    static const char* aname;                   /* (The attacker's name, the defender's: 0 the character) */
    static const char* dname;
    struct fight* f = FIGHT_LB;
    struct msg c;
    unsigned char out = E_FIGHT_OUT (*m), fl = E_FIGHT_FF (*m), pc = !m->who, k;
    const char* aw;
    char* s;

    hub_mem (DEN_BANK, f, FIGHT_AT + E_FIGHT_WORDS (*m) % NFIGHTS, sizeof *f);
    s = pc || (f->which & FW_STRAY) ? f->def : f->att;     /* (A stray's: the one it struck, seen or not) */
    b = begun;
    begun = 0;
    if (b && (!pc || (b & 0x7F) != E_FIGHT_DEF (*m))) {
        say (".");                              /* (Not the blow begun, fight_begin's: its words ended) */
        b = 0;
    }
    if (*s) {
        if (b ? b & 0x80 : !o_fight_seen (s, f->which & FW_STRAY ? 0 : pc ? E_FIGHT_DEF (*m) : m->who)) {
            strcpy (s, "?");                    /* (Not seen: something; a blow begun, as its first words had it) */
            untarget |= !pc && !(f->which & FW_STRAY);  /* (Struck at by it: the character turns, its target let
                                                **   go) */
        }
    }
    aname = f->att[0] ? f->att : 0;
    dname = f->def[0] ? f->def : 0;
    f->text[sizeof f->text - 1] = 0;
    for (k = 0, s = f->text; k < 4; ++k) {      /* (Its four lines, cut apart where they are) */
        line[k] = s;
        while (*s && *s != '\n') {
            ++s;
        }
        if (*s) {
            *s++ = 0;
        }
    }
    aw = line[0];
    said_s[0] = dname;                          /* (Its words, the main RAM's: the record's, in lb) */
    said_s[1] = aw;
    said_s[2] = line[3];
    said_s[3] = aname;
    said_s[4] = line[1];
    said_s[5] = line[2];
    said_n = pc ? BD_TWO : body_of (m->who);    /* (Its getting up: @rises) */
    c.kind = UK_FIGHT;
    c.who = m->who;
    c.a = fl;
    c.b = f->which;
    k = out == FO_GONE ? SAID_FIGHT_GONE : out == FO_BLED ? SAID_FIGHT_BLED : out == FO_BOUND ? SAID_FIGHT_BOUND : out == FO_HURT ? SAID_FIGHT_HURT :
        out != O_RISE && (f->which & FW_STRAY) ? SAID_FIGHT_STRAY : out;
    c.w1 = k << 8 | (!*aw ? 5 : !strcmp (aw, "foot") ? 1 : !strcmp (aw, "head") ? 2 : !strcmp (aw, "bite") ? 3 :
        !strcmp (aw, "claws") ? 4 : 0) | (pc && (cmd.manner & MN_HARD) ? 8 : 0) | (harm_told & C_UNCONSCIOUS ? 16 : 0);
    c.w2 = (aname && *aname == '?' ? 16 : 0) | (dname && *dname == '?' ? 32 : 0);
    if (c.w2 & 16) {                            /* (Its people "they": afolk, dfolk; one not seen, "something", "it" and
                                                **   "its", its actor's 0x40 said.c's they's, 2026-11-17) */
        unthey ((char*) line[3]);
        c.who |= 0x40;
    } else {
        c.w2 |= folk_of (m->who) << 6;
    }
    if (c.w2 & 32) {
        unthey ((char*) line[2]);
    } else {
        c.w2 |= folk_of (E_FIGHT_DEF (*m)) << 7;
    }
    c.w2 |= struck (line[2]) | struck (line[3]) << 2;
    if (b) {
        if (k <= O_CRIT) {
            said_skip = begun_n;                /* (Its first words said as it began: the rest of the sentence) */
        } else {
            say (".");                          /* (No strike after all: its first words ended) */
        }
    }
    outcome_words (&c);
    if ((k >= SAID_FIGHT_GONE && k != SAID_FIGHT_STRAY) || k == O_RISE) {
        return 0;                               /* (No blow: nothing left in) */
    }
    if (!(fl & (FF_A_DEAD | FF_D_DEAD)) && (fl & (FF_A_DOWN | FF_D_DOWN)) && !(f->which & FW_AGAIN) &&
        !(fl & FF_A_DOWN ? aname : dname)) {
        harm_told |= C_UNCONSCIOUS;             /* (The character's black-out: once an answer, then "again", as a harm's
                                                **   state is, harm_told's: a second fall senseless in a long fight's
                                                **   answer, 2026-10-30) */
    }
    return fl & (!aname ? FF_A_DOWN : !dname ? FF_D_DOWN : 0) ? f->out : 0;
}
#pragma static-locals (pop)

/* A word of a fight's, into lb: the stance taken (rs.h's D_*), or why there's no blow (FN_*).  Overlay 22's since
** 2026-11-06 (overlay 6's room: a line that breaks off, parts.c's): fight_note */
enum { FN_AWAY = 5, FN_NONE_LIKE, FN_NONE };
#define note    fight_note

#pragma code-name (push, "OV22CODE")
#pragma rodata-name (push, "OV22RODATA")
#pragma local-strings (push, on)
void __fastcall__ o_fight_note (unsigned char n)
{
    struct msg c;

    if ((n & 7) == FN_AWAY) {           /* ("It is out of reach: go up to it first.": the refusals' one wording; n | 8,
                                        **   a person's, "They are ... them") */
        say_no (A_FAR | A_LB | (n & 8 ? A_THEY : 0), 0);
        return;
    }
    n &= 7;
    said_s[2] = rs_owns;                /* (D_OWN's the ruleset's: RS_HELLO's) */
    c.kind = UK_SELF;                   /* (Its words said.txt's SELF.stance, left in lb, 2026-11-15) */
    c.who = 0;
    c.w1 = MY_STANCE << 8 | n;
    said_q = 1;
    outcome_words (&c);
    said_q = 0;
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The character's stance (the hub's pc_stance, rs.h's D_*: the referee's defence for it, the world's to set: wset)
** set by its word (dodge, block, the ruleset's own (rs_own, RS_HELLO's: HarnMaster's counter; none, ""), none; else
** the best: each by its first letter), told in lb */
static void __fastcall__ stance (const char* w, unsigned char d)
{
    if (*w) {
        d = *w;
        d = d == 'd' ? D_DODGE : (d == 'b' && w[1] == 'l') || d == 'p' ? D_BLOCK : d == *rs_own ? D_OWN : d == 'n' ?
            D_NONE : D_AUTO;                    /* (dodge, duck; block, parry; its own; none; best, else) */
    }
    wset (WS_STANCE, d, 0);
    note (d);
}

/* Something in the character's hands (the UI bank's ix, the world's: CX_HELD): 1, or 0 */
#pragma static-locals (push, off)               /* (Copied from the hub onto the C stack, the main RAM) */
static unsigned char in_hand (void)
{
    struct cix e;
    unsigned char n, k;

    hub_mem (UI_BANK, &n, &UIREC->ixn, 1);
    for (k = 0; k < n && k < NIX; ++k) {
        hub_mem (UI_BANK, &e, &UIREC->ix[k], sizeof e);
        if (e.flags & CX_HELD) {
            return 1;
        }
    }
    return 0;
}

/* A kind of what the character carries named w (the UI bank's ixks: the world's IO_FIND's IR_OK too): 1, or 0 */
static unsigned char __fastcall__ carried (const char* w)
{
    char nm[sizeof UIREC->ixks[0].name + 1];
    unsigned char n, k;

    nm[sizeof nm - 1] = 0;
    hub_mem (UI_BANK, &n, &UIREC->ixk, 1);
    for (k = 0; k < n && k < NIXK; ++k) {
        hub_mem (UI_BANK, nm, UIREC->ixks[k].name, sizeof nm - 1);
        if (name_is (nm, w)) {
            return 1;
        }
    }
    return 0;
}
#pragma static-locals (pop)

/* A thing named w lying in the character's reach, as the senses see it (the world's IO_FIND's IR_OK): 1, or 0 */
static unsigned char __fastcall__ thing_near (const char* w)
{
    const struct sight* s = sight_get ();
    const struct sitem* it = s->item;
    unsigned char k;

    for (k = s->nitems; k; --k, ++it) {
        if (it->x - s->x + 1 <= 2 && it->y - s->y + 1 <= 2 && name_is (it->name, w)) {
            return 1;
        }
    }
    return 0;
}

/* An attack's target found, d (its entry plus 1, | FT_CLOSE out of reach): its first words at once (the user's,
** 2026-11-17: "You close with the giant rat."; out of reach, its walk over's), once a line (sway.c's: looked for as the
** blow's weapon's chosen, wield's, and again as it's carried out).  Looked for as the weapon's chosen (worded's 0x80,
** choose.c's wielded) with its hands empty, a question may come first ("... would you rather draw your sword?", its
** first words): held till the blow's carried out (unasked under dq0 -a) */
static unsigned char __fastcall__ found (const struct cmd* c, unsigned char d)
{
    if (c->action == VB_ATTACK_OBJECT && !(c->flags & CF_LIST)) {  /* (Two foes by turns, a list's: hdens.c's split, its
                                                **   own words) */
        if (worded & 0x80) {
            if (!autogo && !in_hand ()) {
                return d;
            }
            worded = 0;
        }
        if (hold_w[0] && !worded) {             /* (Its weapon named, taken in hand first: its words first, "You lift the
                                                **   sword. You close with the giant rat.", the user's, 2026-11-18) */
            walk_op = IO_HOLD | IO_STEPS;
            walk_ahead (hold_w);
            worded = 0;
        }
        walk_says ((const char*) (W_CLOSE + d));
    }
    return d;
}

/* VB_ATTACK_OBJECT's denizen beside the character to strike at (one whose name has the command's word, if it's
** given; the one that way, a heading, if that is), struck with what the verb says (its aux: SK_*, main.c's strike,
** sent with the blow for the hub's pc_strike): its entry plus 1; or 0, and why not in lb; at a door, a window, a
** portcullis, FT_THING: a blow at it (main.c's: broken).  VB_DEFEND's stance set (the word said, else the verb's:
** "defend dodge", "dodge"): 0 */
unsigned char __fastcall__ o_fight_target (struct cmd* c)
{
    static struct actor q;
    static struct { unsigned char sn; unsigned x, y; } pc;
    const char* w = c->obj.word;
    unsigned char k, away = 0, best = 0, bdist = 0xFF, d, nigh, dir = c->dir;
    int dx, dy, ex, ey;

    if (c->action == VB_DEFEND) {
        stance (w, c->aux);
        return 0;
    }
    if (c->aux) {
        strike = c->aux;                        /* (Sent with the blow, I_ATTACK's x[1]: the verb's part of it; else
                                                **   the thing named, resolve.c's, or the best it holds) */
    }
    if (c->obj.kind == OB_DOOR || c->obj.kind == OB_WINDOW || c->obj.kind == OB_PORTC) {
        return FT_THING;
    }
    hub_mem (0, &pc.sn, (const void*) &HUB->pc_stratum, 1);
    hub_mem (0, &pc.x, (const void*) &HUB->pc_x, 4);
    for (k = 0; k < NACTORS; ++k) {
        hub_mem (0, &q, &HUB->actor[k], sizeof q);
        dx = q.x - pc.x;
        dy = q.y - pc.y;
        if (q.state && q.name[0] && q.sn == pc.sn && name_is (q.name, w) &&
            (dir > H_NW || bearing (dx, dy) == dir)) {
            nigh = dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1;
            if (!c->obj.x) {
                if (nigh) {
                    return found (c, k + 1);
                }
                away = FN_AWAY | (folk_of (k + 1) ? 8 : 0);   /* (Out of reach: a person's, "They are ... them") */
            }
            ex = c->obj.x ? q.x - c->obj.x : dx;   /* (One it sees, said or the target: the one of its name nearest
                                                **   where it was seen; else, several of its name and none beside it,
                                                **   the nearest in sight: struck, or closed with first, I_CLOSE; a
                                                **   missile weapon's shot instead, slice 4.7's) */
            ey = c->obj.x ? q.y - c->obj.y : dy;
            ex = ex < 0 ? -ex : ex;
            ey = ey < 0 ? -ey : ey;
            d = ex > ey ? (ex > 0xFE ? 0xFE : ex) : (ey > 0xFE ? 0xFE : ey);
            if (d < bdist && (c->obj.x || (*w && den_seen (q.x, q.y)))) {
                bdist = d;
                best = nigh ? k + 1 : (k + 1) | FT_CLOSE;
            }
        }
    }
    if (best) {
        return found (c, best);
    }
    d = *w && !c->obj.kind && c->action == VB_ATTACK_OBJECT && !(worded & 0x80);
    if (d && !away && (thing_near (w) || carried (w))) {
        unknown (U_CANT);                       /* ("You can't attack bones.": a thing in reach, not a denizen, refused at
                                                **   once, the world's IO_FIND to come saying nothing, worded 5: do_fight's,
                                                **   refuse.c's, 2026-11-18; or carried, the UI's index's, its IO_FIND's
                                                **   IR_OK known: "attack the rat", the rations, 535,000 cycles of the
                                                **   world's, gw3's, 2026-11-19) */
        worded = 5;
    }
    note (away ? away : *w ? FN_NONE_LIKE : FN_NONE);
    if (d && worded != 5 && !carried (w)) {
        *c->obj.word = 0;                       /* (Nothing of its name in reach nor carried: the world's IO_FIND would
                                                **   find none, its why in lb now, do_fight's, said at once) */
    }
    return 0;
}

/* ---- Grappling (HOUSE.md's grappling, 2026-11-09: the user's "full HarnMaster grappling"): overlay 19's, through their
** stubs (overlay 6's room) */
#pragma code-name (push, "OV19CODE")
#pragma rodata-name (push, "OV19RODATA")
#pragma local-strings (push, on)

/* VB_GRAPPLE_OBJECT's (and a hold's, a throw's at a denizen, | GT_TRY): move g (rs.h's GM_*) at the denizen beside the
** character named (its name has the command's word; the target's, none said), sent (I_ATTACK, b AIM_GRAPPLE | g: the
** world's to strike, the referee's RS_GRAPPLE, fought on till the move's done), with its bare hands (SK_PUNCH: its
** Unarmed); none beside, why not said (one out of reach: "go up to it first", HarnMaster's grapple a melee attack at
** one engaged).  "Let go": the hold let go, the world's.  GR_SEND | the actor plus 1 (0 none: a let go), its intent's
** b in intend_a; GR_SAID, why not in lb; GT_TRY's 0 if none of that name is beside, nothing said */
static unsigned char gk;                        /* (The denizen grappled last, plus 1) */
static char an[sizeof HUB->actor[0].name];      /* (One of the name out of reach) */

unsigned char __fastcall__ o_grapple (unsigned char g)
{
    struct actor q;                             /* (On the C stack) */
    unsigned char sn, k, f, away = 0;
    unsigned px, py;
    const char* w = cmd.obj.word;
    const char* s;

    if (g == GM_HOLD) {                         /* (The grapple verb's: its move the word's, the line's first: pin;
                                                **   choke, strangle, throttle; tackle; else a hold) */
        for (s = cl; *s == ' '; ++s) {
        }
        k = *s | 0x20;
        g = k == 'p' ? GM_PIN : k == 'c' || k == 's' || (k == 't' && (s[1] | 0x20) == 'h') ? GM_CHOKE : k == 't' ?
            GM_THROW : GM_HOLD;
    }
    if (g == GM_LETGO) {
        k = 0;
        goto sent;
    }
    if ((g & GT_TRY) && (!*w || cmd.obj.kind)) {
        return 0;
    }
    hub_mem (0, &sn, (const void*) &HUB->pc_stratum, 1);
    hub_mem (0, &px, (const void*) &HUB->pc_x, 2);
    hub_mem (0, &py, (const void*) &HUB->pc_y, 2);
    for (f = k = 0; k < NACTORS; ++k) {
        hub_mem (0, &q, &HUB->actor[k], sizeof q);
        if (q.state && q.name[0] && q.sn == sn && name_is (q.name, w)) {
            if ((unsigned) (q.x - px + 1) > 2 || (unsigned) (q.y - py + 1) > 2) {
                hub_mem (0, &away, &HUB->folk[k], 1);
                away = away ? A_THEY : 1;       /* (A person's: "go up to them first") */
                memcpy (an, q.name, sizeof an);   /* (Its name, said: "The giant rat is out of reach") */
            } else if (!f || k + 1 == gk) {
                f = k + 1;                      /* (The first beside; the one grappled last before it, the one held:
                                                **   "choke the rat" the rat it pinned) */
            }
        }
    }
    if (f) {
        strike = SK_PUNCH;                      /* (Its bare hands: its Unarmed, the referee's) */
        gk = k = f;
        goto sent;
    }
    if (g & GT_TRY) {
        return 0;
    }
    if (away) {
        *lb = 0;
        say_no (A_FAR | A_LB | (away & A_THEY), an);    /* ("The giant rat is out of reach: go up to it first.") */
    } else {
        fight_note (*w ? 6 : 7);                /* ("There's nothing like that here to fight.") */
    }
    return GR_SAID;
sent:
    intend_a = AIM_GRAPPLE | (g & 7);           /* (Sent by the caller, hdens.c's grip_go: an intent maps the hub into
                                                **   the window, an overlay's) */
    return k | GR_SEND;
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
