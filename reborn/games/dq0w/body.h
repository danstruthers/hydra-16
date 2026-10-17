/*
** body.h - (dq0w) a portal's body (DUNGEON.md: portals): the net harm a leaf (a door, shutters, a portcullis's bars)
** takes before it fails, by its material (PM_*), its size (feet, world.h's), its thickness (inches) and its state (0
** rotten ... 9 new); a window's bars half a plate's.  The maker's as it makes them (dq0m/fixtures.c), the world's as
** they're battered (wblow.c): each includes it, its copy in its own code.
*/

static unsigned char __fastcall__ part_body (unsigned char mat, unsigned char size, unsigned char thick,
    unsigned char qual)
{
    static const unsigned char per[4] = { 2, 3, 6, 4 };    /* (PM_*: a square foot an inch thick, in quarters) */
    unsigned b = (size & 15) * (size >> 4) * thick * per[mat & 3] / 4 * (2 + qual) / 6;

    return b > 255 ? 255 : b ? b : 1;
}
