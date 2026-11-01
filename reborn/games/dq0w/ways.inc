/* ways.inc - (the world's; chunk.c includes it, and the senses' view.c a copy of its own, renamed, for part 4) a
** passage's breadth, a way on, the ways on from a square: chunk.c's breadth, way_ok, ways_from */

unsigned char __fastcall__ breadth (unsigned char sn, unsigned x, unsigned y, unsigned char d)
{
    unsigned char n, e, a, b, u, v;
    unsigned nx, ny;

    for (n = 0; n < 2; ++n) {
        nx = x + dir_x[d];
        ny = y + dir_y[d];
        e = (d + 1) & 3;
        a = step_ok (sn, x, y, e);
        b = step_ok (sn, x, y, e ^ 2);
        u = step_ok (sn, nx, ny, e);
        v = step_ok (sn, nx, ny, e ^ 2);
        if (!((a && u) || (b && v)) || (u && !a) || (v && !b)) {
            return 0;                           /* (Not alongside, or a way out of it this one hasn't: a way off) */
        }
        if (!step_ok (sn, nx, ny, d)) {
            return 1;                           /* (Its far side: the passage's breadth) */
        }
        x = nx;
        y = ny;
    }
    return 0;                                   /* (Wider than three: a way) */
}

/* A way on from (x, y) heading h: one of the four walkable and not a wide passage's breadth, or a diagonal way */
unsigned char __fastcall__ way_ok (unsigned char sn, unsigned x, unsigned y, unsigned char h)
{
    return h & 1 ? slant (sn, x, y, h) : step_ok (sn, x, y, h >> 1) && !breadth (sn, x, y, h >> 1);
}

/* The ways on from (x, y) heading h (a walk's: of the eight, none behind, nor a wide passage's breadth, nor an opening
** into the room the square before opened into, way_room: a passage along a room's side one way into it; a diagonal a
** diagonal way's, slant's): how many; one of them in way_on (its heading); the room it opens into now in way_room */
unsigned char way_on, way_room = 0xFF;
unsigned char __fastcall__ ways_from (unsigned char sn, unsigned x, unsigned y, unsigned char h)
{
    unsigned char k, nd = 0, r, rm = 0xFF;

    for (k = 0; k < 8; ++k) {
        if (!REL_BEHIND (k - h) && way_ok (sn, x, y, k)) {
            if ((r = room_of (sn, x + head_x[k], y + head_y[k])) != 0xFF && (rm = r) == way_room) {
                continue;
            }
            ++nd;
            way_on = k;
        }
    }
    way_room = rm;
    return nd;
}
