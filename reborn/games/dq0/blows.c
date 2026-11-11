/*
** blows.c - (overlay 6) a fight's words: a blow struck (an E_FIGHT: the character's at a denizen, or one's at it), how
** it came out (rs.h's O_*), what the strikes did (the referee's words: the wounds, or the armour that stopped them),
** and what it left them in (fallen, senseless, dead, fleeing): "You strike at the giant rat with your sword: a
** serious cut to its left thigh (S2). The giant rat falls senseless."  Into lb, for main.c to say (the blow's record
** copied into lb's last FIGHT_LB bytes: the words never as long as the rest).
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <string.h>
#include "hub.h"
#include "rs.h"
#include "dq0.h"
#include "cmds.h"

static char* t;                                 /* (Where the next words go in lb) */
static const char* line[4];                     /* The referee's lines: the attacker's weapon, the defender's, what the
                                                **   attacker's strike did, what the defender's did */
static const char* aname;                       /* The attacker's name, the defender's (0: the character) */
static const char* dname;

static void __fastcall__ add (const char* s)
{
    strcpy (t, s);
    t += strlen (t);
}

/* One of them: "you" (or "You"), "the giant rat" (or "The") */
static void __fastcall__ who (const char* n, unsigned char cap)
{
    if (!n) {
        add (cap ? "You" : "you");
        return;
    }
    if (*n == '?') {
        add (cap ? "Something" : "something");  /* (Not seen) */
        return;
    }
    add (cap ? "The " : "the ");
    add (n);
}

/* A strike's words (its line: a wound, "a ..."; or the armour that stopped it, "your ...", "its ..."; or none: no
** harm) after what led to it: ": a serious cut ...", ", but the blow glances off your leather tunic", ", but does no harm"
** (by the striker: the character's "do") */
/* A creature's getting up in a fight (the book's Rise), by its body (hub.h's BD_*, the actor's: kinds.txt's); its fall
** told after the blow, the world's (E_BEAT's WB_FALLEN: said.txt's BEAT.fallen, every fall, 2026-10-30) */
static const char* const rises[NBODIES] = { " gets to its feet.", " scrambles to its feet.", " coils itself up again.",
    " rights itself.", " flutters up again.", " rises again." };
static unsigned char __fastcall__ body_of (unsigned char k)
{
    unsigned char b;

    hub_mem (0, &b, &HUB->actor[(k - 1) & 7].body, 1);
    return b < NBODIES ? b : BD_TWO;
}

static void __fastcall__ struck (unsigned char n, unsigned char you)
{
    const char* l = line[n];

    if (*l == 'a' && l[1] == ' ') {
        add (": ");
        add (l);
    } else if (*l) {
        add (", but the blow glances off ");    /* (Whatever its number: "your clothes", 2026-11-04) */
        add (l);
    } else {
        add (you ? ", but do no harm" : ", but does no harm");
    }
}

/* A blow (an E_FIGHT: the character's, or one at it), worded; the character dead of it, the game's end; knocked
** senseless, it waits it out (main.c's till 2026-11-03: the main RAM's room) */
unsigned __fastcall__ o_fight_words (const struct msg* m);
void __fastcall__ o_blow (const struct msg* m)
{
    unsigned out = o_fight_words (m);
    unsigned char c;

    msg (lb);
    hub_mem (0, &c, (const void*) &HUB->pcsum.cond, 1);
    if (c & C_DEAD) {
        ending (END_DIED);                         /* (The game's end, as harm's is) */
    } else if (out) {
        intend2 (I_WAIT, out, I_WAIT_W2 (0, 0, 1));   /* (Out: the time it's senseless) */
    }
}

unsigned __fastcall__ o_fight_words (const struct msg* m)
{
    static unsigned char n;                     /* (What the character sees: whether it sees the denizen, or one of its
                                                **   name: the senses' last look, a step behind it) */
    static char at[12];
    struct fight* f = FIGHT_LB;
    unsigned char out = E_FIGHT_OUT (*m), fl = E_FIGHT_FF (*m), pc = !m->who, k;
    const char* aw;
    char* s;

    hub_mem (DEN_BANK, f, FIGHT_AT + E_FIGHT_WORDS (*m) % NFIGHTS, sizeof *f);
    hub_mem (SIGHT_BANK, &n, &SIGHT->ndens, 1);
    s = pc || (f->which & FW_STRAY) ? f->def : f->att;     /* (A stray's: the one it struck, seen or not) */
    if (*s) {
        for (k = n; k--; ) {
            hub_mem (SIGHT_BANK, at, SIGHT->den[k].name, sizeof at);
            if (!strcmp (at, s)) {
                break;
            }
        }
        if (k == 0xFF) {
            strcpy (s, "?");                    /* (Not seen: something) */
            untarget |= !pc && !(f->which & FW_STRAY);  /* (Struck at by it: the character turns, its target let
                                                **   go) */
        }
    }
    t = lb;
    *t = 0;
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
    if (out == FO_GONE) {
        who (dname, 1);                         /* ("The giant rat", or "Something" if it isn't seen) */
        add (" is out of reach.");
        return 0;
    }
    if (out == FO_BLED || out == FO_BOUND) {
        who (aname, 1);
        add (out == FO_BLED ? " collapses, bled white." : " binds its bleeding wound.");
        return 0;
    }
    if (out == O_RISE) {
        who (aname, 1);
        add (pc ? " get to your feet." : rises[body_of (m->who)]);
        return 0;
    }
    if (f->which & FW_STRAY) {                  /* (A miss's stray at one beside the mark, after the miss's words:
                                                **   "Your shot strikes the bandit instead: a minor ...", "Its dagger
                                                **   strikes you instead, but the blow glances off your mail hauberk",
                                                **   2026-10-30) */
        add (pc ? "Your " : "Its ");
        add (f->which & FW_THROWN ? aw : "shot");
        add (" strikes ");
        who (dname, 0);
        add (" instead");
        struck (2, 0);
        add (".");
    } else if (out >= O_HOLD) {
        grip_words (out);                       /* (A grapple's: overlay 19's, 2026-11-09) */
    } else if (out == O_DISPATCH) {
        add ("You finish ");
        who (dname, 0);
        add (" where it lies.");
    } else {
        if (f->which & FW_FIRST) {
            add ("Quicker, ");                  /* (A tactical advantage's blow, struck first: the engage initiative
                                                **   test's, "Quicker, the giant rat bites at you") */
        }
        who (aname, !(f->which & FW_FIRST));    /* (The blow: "You strike at the giant rat with your sword", "The
                                                **   ghoul claws at you", "The kobold strikes at you with its spear") */
        if (f->which & FW_THROWN) {             /* (A missile's, slice 4.7's: "You throw your dagger at the giant
                                                **   rat", "You shoot at the bandit with your shortbow", "The kobold
                                                **   shoots at you with its sling") */
            add (pc ? " throw your " : " throws its ");
            add (aw);
            add (pc && (cmd.manner & MN_HARD) ? " hard at " : " at ");
            who (dname, 0);
        } else if (f->which & FW_MISSILE) {
            add (pc ? " shoot at " : " shoots at ");
            who (dname, 0);
            if (*aw) {
                add (pc ? " with your " : " with its ");
                add (aw);
            }
        } else if (pc && !strcmp (aw, "foot")) {
            add (" kick at ");                  /* ("You kick at the giant rat") */
            who (dname, 0);
        } else if (pc) {
            add (strcmp (aw, "head") ? " strike at " : " butt at ");
            who (dname, 0);
            add (" with your ");
            add (aw);
        } else if (!*aw) {
            add (" strikes at you");            /* (A ruleset that names no weapon: the null one's) */
        } else if (!strcmp (aw, "bite") || !strcmp (aw, "claws")) {
            add (*aw == 'b' ? " bites at you" : " claws at you");
        } else {
            add (" strikes at you with its ");
            add (aw);
        }
        switch (out) {
        case O_MISS:
            add (pc ? ", and miss" : ", and misses");
            break;
        case O_DODGED:
            add (pc ? "; it dodges" : "; you dodge");
            break;
        case O_BLOCKED:
            add (pc ? "; it fends off the blow" : "; you fend it off with your ");
            if (pc && (f->which & FW_SHIELD)) {
                add (" with its ");            /* (Its shield's block: "with its buckler", 2026-10-29) */
            }
            if (!pc || (f->which & FW_SHIELD)) {
                add (line[1]);
            }
            if (*line[2]) {                     /* (A bare arm struck as it blocked) */
                struck (2, pc);
            } else if (*line[3]) {              /* (A claw, a bite, struck by what blocked it) */
                add (pc ? "; it strikes you as it does" : "; your blow lands on it");
                struck (3, 0);
            }
            break;
        case O_HIT:
            struck (2, pc);
            break;
        case O_COUNTER_HIT:
            add (pc ? "; it strikes back" : "; you strike back");   /* (A counterstrike: "back", not "first", for a
                                                **   blow struck first, the engage initiative test's, may meet one) */
            struck (3, !pc);
            break;
        case O_BOTH_HIT:
            add (pc ? "; you strike each other. Your blow" : "; you strike each other. Its blow");
            struck (2, 0);
            add (pc ? "; its" : "; yours");
            struck (3, 0);
            break;
        case O_FUMBLE:
        case O_STUMBLE:
            k = f->which;
            if (k & 1) {
                add (out != O_FUMBLE ? (pc ? ", but stumble" : ", but stumbles") : f->which & FW_MISSILE ?
                    (pc ? ", but your shot goes wild" : ", but its shot goes wild") :
                    pc ? ", but fumble the blow" : ", but fumbles the blow");
            }
            if (k & 2) {
                add (out == O_FUMBLE ? (pc ? "; it falters" : "; you fumble your guard") :
                    pc ? "; it stumbles" : "; you stumble");
            }
            break;
        case O_ADVANTAGE:
            add (pc ? "; it slips inside your guard" : "; you see an opening");
            break;
        }
        add (".");
    }
    if (fl & (FF_A_DEAD | FF_D_DEAD)) {         /* (What it left them in: the character's, its name none; both
                                                **   actors a stray's) */
        add (" ");
        aw = fl & FF_A_DEAD ? aname : dname;
        who (aw, 1);
        add (aw ? " falls dead." : " are dead.");
    } else if ((fl & (FF_A_DOWN | FF_D_DOWN)) && !(f->which & FW_AGAIN)) {
        if (!(fl & FF_A_DOWN ? aname : dname)) {   /* (The character's: once an answer, then "again", as a harm's state
                                                **   is, harm_told's: a second fall senseless in a long fight's
                                                **   answer, 2026-10-30) */
            add (harm_told & C_UNCONSCIOUS ? " Everything goes black again." : " Everything goes black.");
            harm_told |= C_UNCONSCIOUS;
        } else {
            add (" It falls senseless.");
        }
    }
    if (fl & FF_FLEES) {
        add (" It flees.");
    }
    return fl & (!aname ? FF_A_DOWN : !dname ? FF_D_DOWN : 0) ? f->out : 0;
}

/* A word of a fight's, into lb: the stance taken (rs.h's D_*), or why there's no blow (FN_*).  Overlay 22's since
** 2026-11-06 (overlay 6's room: a line that breaks off, parts.c's): fight_note */
enum { FN_AWAY = 5, FN_NONE_LIKE, FN_NONE };
#define note    fight_note

#pragma code-name (push, "OV22CODE")
#pragma rodata-name (push, "OV22RODATA")
#pragma local-strings (push, on)
void __fastcall__ o_fight_note (unsigned char n)
{
    static const char* const say[8] = {
        "You will defend yourself as best you can.", "You will block.", "You will dodge.",
        "You will strike back first.", "You will not defend yourself.", "",
        "There's nothing like that here to fight.", "There's nothing beside you to fight."
    };

    strcpy (lb, say[n & 7]);
    if ((n & 7) == 5) {
        say_no (A_FAR | A_LB, 0);       /* ("It is out of reach: go up to it first.": the refusals' one wording) */
    }
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The character's stance (the hub's pc_stance, rs.h's D_*: the referee's defence for it, the world's to set: wset)
** set by its word (dodge, block, counter, none; else the best), told in lb */
static void __fastcall__ stance (const char* w, unsigned char d)
{
    if (*w) {
        d = *w;
        d = d == 'd' ? D_DODGE : (d == 'b' && w[1] == 'l') || d == 'p' ? D_BLOCK : d == 'c' ? D_OWN : d == 'n' ?
            D_NONE : D_AUTO;                    /* (dodge, duck; block, parry; counter; none; best, else) */
    }
    wset (WS_STANCE, d, 0);
    note (d);
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
                    return k + 1;
                }
                away = 1;
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
        return best;
    }
    note (away ? FN_AWAY : *w ? FN_NONE_LIKE : FN_NONE);
    return 0;
}

/* ---- Grappling (HOUSE.md's grappling, 2026-11-09: the user's "full HarnMaster grappling"): overlay 19's, through their
** stubs (overlay 6's room) */
#pragma code-name (push, "OV19CODE")
#pragma rodata-name (push, "OV19RODATA")
#pragma local-strings (push, on)

/* A grapple's words at t (rs.h's O_HOLD ... O_LETGO): the other in it its name, % ("the giant rat", "something" if
** it isn't seen), @ at the start ("The giant rat"): the character's move's defender, the held one's own try's
** attacker; a throw's hurt after it, the referee's third line */
void __fastcall__ o_grip_words (unsigned char out)
{
    static const char* const say[] = {
        "You seize % and hold it fast.", "You grapple with %, but it twists free.", "You throw % to the ground",
        "You pin % to the ground.", "You choke %.", "@ resists your hold.", "@ breaks free of your grip.",
        "@ struggles in your grip.", "You let go of %."
    };
    const char* s = say[out - O_HOLD];
    const char* n = out == O_FREE || out == O_STRUGGLES ? aname : dname;
    char c;

    if (!n) {
        s = "You aren't holding anyone.";       /* (O_LETGO's, none held) */
    }
    while ((c = *s++) != 0) {
        if (c == '%' || c == '@') {
            strcpy (t, *n == '?' ? (c == '@' ? "Something" : "something") : c == '@' ? "The " : "the ");
            if (*n != '?') {
                strcat (t, n);
            }
            t += strlen (t);
        } else {
            *t++ = c;
        }
    }
    *t = 0;
    if (out == O_THROWN) {
        if (*line[2] == 'a' && line[2][1] == ' ') {
            strcpy (t, ": ");
            strcat (t, line[2]);
            t += strlen (t);
        }
        strcpy (t++, ".");
    }
}

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
                away = 1;
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
        say_no (A_FAR | A_LB, an);              /* ("The giant rat is out of reach: go up to it first.") */
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
