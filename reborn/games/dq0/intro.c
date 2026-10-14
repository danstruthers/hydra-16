/*
** intro.c - (overlay 1) Dingus Quest Zero's title and its pace asked, and the character's profile into the story
** (the sheet command).  The character itself is made by hmchar (main.c runs it).
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"

void o_title (void)
{
    blank ();
    msg ("D I N G U S   Q U E S T   Z E R O");
    blank ();
    msg ("A dungeon, made new for each descent, and a puzzle in pieces somewhere in it: find them, and you may yet "
         "leave the place, or rule it.  The dungeon doesn't wait while you think.");
    blank ();
    if (pace == 0xFF) {
        msg ("How fast does the dungeon move?  (r)elaxed, (n)ormal or (m)enacing: ");
        pace = ask ("rnm");
        say (pacename[pace]);
    }
    blank ();
}

/* A character's file (hmchar's) read: its own lines are the name and the occupation.  0, or -1 */
static void __fastcall__ own_line (char* l)
{
    if (!strncmp (l, "name ", 5)) {
        strncpy (P.name, l + 5, sizeof P.name - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (P.occupation, l + 11, sizeof P.occupation - 1);
    }
}

int __fastcall__ o_load (const char* file)
{
    int fd = open (file, O_RDONLY), r;

    if (fd < 0) {
        return -1;
    }
    r = harn_read (fd, &P.h, own_line);
    close (fd);
    return r;
}

static const unsigned char shown[] = {
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR
};

void o_sheet (void)
{
    harn_char* c = &P.h;
    unsigned char k;

    blank ();
    sayf ("%s, %s %s, %s; born %u %s (%s).", P.name, harn_species_name (c->att[HARN_SP]),
        harn_sex_name (c->att[HARN_SX]), P.occupation, c->att[HARN_BD] & 0xFF,
        harn_month_name (c->att[HARN_BD] >> 8), harn_sunsign_name (c->att[HARN_SS]));
    msg ("");
    for (k = 0; k < sizeof shown; ++k) {
        sayf ("%s %u%s ", harn_attr_name (shown[k]), c->att[shown[k]], k + 1 < sizeof shown ? "," : ".");
    }
    msg ("");
    sayf ("Endurance %u, Move %u; universal penalty %u, physical %u.", harn_end (c), harn_move (c), harn_get_up (c),
        harn_get_pp (c));
    msg ("Skills (EML):");
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (c->ml[k]) {
            sayf (" %s %d", harn_skill_name (k), harn_eml (c, k));
        }
    }
    blank ();
}
