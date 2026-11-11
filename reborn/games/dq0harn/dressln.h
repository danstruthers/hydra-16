/*
** dressln.h - (dq0harn) the character's file's "dressed" line, read and written: the referee's (dress.c's o_dress_line,
** overlay 9: part 1's, harnfile's, its room) and the account's between dungeons (../dq0acct's sheet.c, whose days soil
** them: the user's, 2026-11-11).  The includer has pc and pc_dr, and names the function DRESS_LINE.
**
** After the word, its dressings, each its grade plus 1 (a digit, ':' 10; soiled, a letter, 'B' 1), where it is (two
** letters, its location's high four bits and low: "8af"), and its cloth if it's known (a letter after them, 'q' plus its
** DM_*: r hemp, s linen, t wool, u silk; none, a file's from before it was kept: "8afs").  Read (out 0: each the first
** wound there undressed's, its injuries read before it: the account's days at rest may have healed some), or written
** into l (out 1: "" none; 4 bytes a dressing at most)
*/

void __fastcall__ DRESS_LINE (char* l, unsigned char out)
{
    unsigned char k, g, m;

    if (out) {
        for (k = 0; k < pc.ninj; ++k) {
            if ((g = pc_dr[k]) != 0) {
                *l++ = (g & DR_SOILED ? 'A' : '0') + (g & DR_GRADE);
                *l++ = 'a' + (pc.inj[k].loc >> 4);
                *l++ = 'a' + (pc.inj[k].loc & 15);
                if (g & DR_MAT) {
                    *l++ = 'q' + ((g & DR_MAT) >> 4);
                }
            }
        }
        *l = 0;
        return;
    }
    for (; l[0] && l[1] && l[2]; l += 3) {
        g = (l[1] - 'a') << 4 | (l[2] - 'a');
        m = (unsigned char) (l[3] - 'r') < 4 ? (l[3] - 'q') << 4 : 0;
        for (k = 0; k < pc.ninj && (pc.inj[k].loc != g || pc_dr[k]); ++k) {
        }
        if (k < pc.ninj && (unsigned char) (l[0] - '1') < 10) {
            pc_dr[k] = (l[0] - '0') | m;
        } else if (k < pc.ninj && (unsigned char) (l[0] - 'B') < 10) {
            pc_dr[k] = (l[0] - 'A') | DR_SOILED | m;
        }
        l += m != 0;
    }
}
