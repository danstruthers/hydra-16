/*
** sketch.c - the Vera X's screen (vera.h): a bitmap under the console, a frame of boxes and circles, then the mouse
** drawing (its left button down: lines after it, in the colour picked from the strip at the top; the right button:
** the drawing cleared), till a key ends it, the bitmap gone.  sketch [DEPTH] (1, 2, 4 or 8 bits a pixel; 8 as it
** starts)
**   % sketch
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <hydra.h>
#include <vera.h>

#define W   320
#define H   240

static void frame (unsigned char colours)
{
    unsigned char c;

    vid_pen (0);
    vid_clear ();
    for (c = 0; c < colours && c < 16; ++c) {       /* The strip: the colours to pick */
        vid_pen (c);
        vid_bar (c * 20, 0, c * 20 + 19, 9);
    }
    vid_pen (15 & (colours - 1));
    vid_box (0, 12, W - 1, H - 1);
    vid_circle (W / 2, H / 2 + 6, 60);
    vid_disc (W / 2, H / 2 + 6, 4);
}

int main (int argc, char* argv[])
{
    unsigned char depth = argc > 1 ? atoi (argv[1]) : 8;
    unsigned char colours = depth >= 4 ? 16 : 1 << depth;
    unsigned char b, was = 0;
    int x, y, lx = 0, ly = 0;

    if (vid_bitmap (W, depth) < 0) {
        perror ("sketch: no bitmap (a Vera X?)");
        hy_exits ("no screen");
    }
    frame (colours);
    vid_pen (colours - 1);
    while (!kbhit ()) {
        if (vera_wait_frame () < 0 || vid_mouse (&x, &y, &b) < 0) {
            break;
        }
        if (b & 4) {                                /* The right button: the drawing cleared */
            frame (colours);
        } else if (b & 1) {
            if (y < 10) {                           /* The strip: a colour */
                vid_pen (x / 20);
            } else if (was & 1) {
                vid_line (lx, ly, x, y);
            } else {
                vid_plot (x, y);
            }
        }
        was = b;
        lx = x;
        ly = y;
    }
    if (kbhit ()) {
        cgetc ();
    }
    vid_bitmap (0, 0);
    return 0;
}
