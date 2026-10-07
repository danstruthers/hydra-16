/*
** vera.h - the Vera X's screen: its driver's files (vid: #v, at /dev/vid), and the chip itself for a program that
** claims it.  (The programmer's chapter: docs/programming/video.md.)
**
** Drawing, with the console still on the screen: vid_bitmap puts a bitmap under the console's text (layer 0: 320 or
** 640 pixels across, 1, 2, 4 or 8 bits a pixel; 640's 1 or 2), and the vid_ calls draw on it, the driver doing the
** drawing (/dev/vid/draw): a pen (its colour: the driver's, one for every program), points, lines, boxes, bars
** (filled), circles and discs (filled), the bitmap cleared, text in the console's font (8 x 8).  Coordinates are
** its pixels, -4096 to 4095 (what falls off it isn't drawn).  vid_palette, vid_sprite, vid_sprite_at and vid_sprite_off write the palette and the
** sprites' attributes; vera_write, vera_read and vera_load VRAM (a 17-bit address); vera_wait_frame waits for the
** next frame (59.5 a second); vid_mouse and vid_mouse_wait read the mouse (Plan 9's: x and y in the screen's pixels,
** the buttons 1 left, 2 middle, 4 right).  Each is a request to the driver; they return 0, or -1 with errno set
** (EINVAL: a bad number, or no bitmap; EBUSY: the chip's claimed).
**
** The chip itself: vera_claim makes it the program's (every register; with all, every byte of VRAM too) till
** vera_release or its end, the console's output waiting meanwhile.  Then VERA is its registers (cc65's cx16.h's
** layout, at $FF20) and vpoke and vpeek its VRAM, as on the X16.  While it's claimed, the vid_ drawing calls get
** EBUSY: the program draws for itself.
**
** cc65's TGI (tgi.h) draws there too: tgi_install (hydra_tgi), then tgi_init () (320 x 240, 256 colours: the
** palette's entries).
*/

#ifndef _VERA_H
#define _VERA_H

#include <hydracalls.h>

/* The registers ($FF20-$FF3F: slot 0's ports 2 and 3), as cc65's cx16.h lays them out */
struct __vera {
    unsigned            address;        /* ADDRx_L, ADDRx_M: the port's address, its low 16 bits */
    unsigned char       address_hi;     /* ADDRx_H: bit 16, DECR, the increment (VERA_INC_*) */
    unsigned char       data0;          /* DATA0, DATA1: a byte at ADDR0, ADDR1 */
    unsigned char       data1;
    unsigned char       control;        /* CTRL: reset, DCSEL, ADDRSEL */
    unsigned char       irq_enable;     /* IEN */
    unsigned char       irq_flags;      /* ISR */
    unsigned char       irq_raster;     /* IRQLINE_L (a write), SCANLINE_L (a read) */
    union {
        struct {                        /* DCSEL 0 */
            unsigned char video;        /* DC_VIDEO */
            unsigned char hscale;
            unsigned char vscale;
            unsigned char border;
        };
        struct {                        /* DCSEL 1 */
            unsigned char hstart;
            unsigned char hstop;
            unsigned char vstart;
            unsigned char vstop;
        };
    } display;
    struct {
        unsigned char   config;
        unsigned char   mapbase;
        unsigned char   tilebase;
        unsigned        hscroll;
        unsigned        vscroll;
    } layer0;
    struct {
        unsigned char   config;
        unsigned char   mapbase;
        unsigned char   tilebase;
        unsigned        hscroll;
        unsigned        vscroll;
    } layer1;
    struct {
        unsigned char   control;
        unsigned char   rate;
        unsigned char   data;
    } audio;
    struct {
        unsigned char   data;
        unsigned char   control;
    } spi;
};
#define VERA                (*(volatile struct __vera *)0xFF20)

#define VERA_INC_0          0x00        /* ADDRx_H's increments */
#define VERA_INC_1          0x10
#define VERA_INC_2          0x20
#define VERA_INC_4          0x30
#define VERA_INC_8          0x40
#define VERA_INC_16         0x50
#define VERA_INC_32         0x60
#define VERA_INC_64         0x70
#define VERA_INC_128        0x80
#define VERA_INC_256        0x90
#define VERA_INC_512        0xA0
#define VERA_INC_40         0xB0
#define VERA_INC_80         0xC0
#define VERA_INC_160        0xD0
#define VERA_INC_320        0xE0
#define VERA_INC_640        0xF0
#define VERA_DECR           0x08
#define VERA_PSG_BASE       0x1F9C0UL   /* VRAM: the PSG's registers ... */
#define VERA_PALETTE_BASE   0x1FA00UL   /*   the palette (2 bytes an entry: $GB, $0R) ... */
#define VERA_SPRITES_BASE   0x1FC00UL   /*   the sprites' attributes (8 bytes a sprite) */
#define VERA_PROGRAM_TOP    0x1AFFFUL   /* The VRAM a claimer may use without claiming all: 0 to here */

/* The chip: claimed, released; ctl's commands; the frames */
int __fastcall__ vera_claim (unsigned char all);                        /* all: every byte of VRAM too */
int vera_release (void);
int __fastcall__ vera_ctl (const char* command);                        /* A line for /dev/vid/ctl: "mode 40x30" */
int vera_wait_frame (void);                                             /* The next frame (59.5 a second) */

/* VRAM directly (the chip claimed): ADDR0, no increment */
void __fastcall__ vpoke (unsigned char data, unsigned long addr);
unsigned char __fastcall__ vpeek (unsigned long addr);

/* VRAM through /dev/vid/vram (the chip not claimed, or claimed) */
int __fastcall__ vera_write (unsigned long addr, const void* buf, unsigned n);
int __fastcall__ vera_read (unsigned long addr, void* buf, unsigned n);
int vera_load (const char* path, unsigned long addr);                   /* A file's bytes into VRAM */

/* The bitmap, and drawing on it (vid's /dev/vid/draw) */
int __fastcall__ vid_bitmap (unsigned width, unsigned char depth);      /* width 0: off */
int __fastcall__ vid_pen (unsigned char colour);
int __fastcall__ vid_plot (int x, int y);
int __fastcall__ vid_line (int x0, int y0, int x1, int y1);
int __fastcall__ vid_box (int x0, int y0, int x1, int y1);
int __fastcall__ vid_bar (int x0, int y0, int x1, int y1);              /* Filled */
int __fastcall__ vid_circle (int x, int y, int r);
int __fastcall__ vid_disc (int x, int y, int r);                        /* Filled */
int vid_clear (void);
int __fastcall__ vid_text (int x, int y, const char* s);                /* The console's font, 8 x 8 */

/* The palette (rgb: 0xRGB, 4 bits each) and the sprites' attributes (8 bytes a sprite) */
int vid_palette (unsigned char index, unsigned rgb);
int vid_sprite (unsigned char n, const unsigned char* attr);
int vid_sprite_at (unsigned char n, int x, int y);
int __fastcall__ vid_sprite_off (unsigned char n);

/* The mouse: as it is, or its next change */
int vid_mouse (int* x, int* y, unsigned char* buttons);
int vid_mouse_wait (int* x, int* y, unsigned char* buttons);

/* cc65's TGI: the Vera X's driver (static: tgi_install (hydra_tgi)) */
extern unsigned char hydra_tgi[];

#endif
