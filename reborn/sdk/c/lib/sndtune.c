/*
** sndtune.c - snd_note_of() and snd_tune(): notes by their names, and a tune of them played, as HyForth's note-of
** and tune and hylang's are.  (A module of its own, so a program that plays notes by number doesn't bring it.)
*/

#include <stdlib.h>
#include <errno.h>
#include <hydra.h>
#include <snd.h>

static const unsigned char semitones[] = { 9, 11, 0, 2, 4, 5, 7 };    /* A B C D E F G */

/* The note named by the n characters at s: its MIDI number, or -1 */
static int __fastcall__ note_n (const char* s, unsigned char n)
{
    unsigned char i = 0, letter;
    int note;

    if (n < 2) {
        return -1;
    }
    letter = (s[i++] & 0xDF) - 'A';                 /* Its letter: its semitone in the octave */
    if (letter > 6) {
        return -1;
    }
    note = semitones[letter];
    if (s[i] == '#') {                              /* # or b */
        ++note;
        ++i;
    } else if (s[i] == 'b') {
        --note;
        ++i;
    }
    if (i + 1 == n && s[i] >= '0' && s[i] <= '9') { /* Its octave, 0-9 ... */
        note += (s[i] - '0' + 1) * 12;
    } else if (!(i + 2 == n && s[i] == '-' && s[i + 1] == '1')) {
        return -1;                                  /*   or -1 (its notes 0-11) */
    }
    return note < 0 || note > 127 ? -1 : note;
}

int __fastcall__ snd_note_of (const char* name)
{
    unsigned char n = 0;
    int note;

    while (name[n] && n < 8) {
        ++n;
    }
    note = name[n] ? -1 : note_n (name, n);
    if (note < 0) {
        return _directerrno (EINVAL);
    }
    return note;
}

/* The next word of a tune, from *p: its start (*p past it), its length in *n; 0 if none is left */
static const char* __fastcall__ word (const char** p, unsigned char* n)
{
    const char* s = *p;
    const char* w;

    while (*s == ' ' || *s == '\t' || *s == '\n') {
        ++s;
    }
    if (!*s) {
        return 0;
    }
    w = s;
    while (*s && *s != ' ' && *s != '\t' && *s != '\n') {
        ++s;
    }
    *p = s;
    *n = s - w;
    return w;
}

/* Each note keyed on at its beat, in step with the system's tick (from the next but one, so the first isn't late),
** the one before keyed off just before it; the beats added up as they go, so the tune doesn't drift */
int __fastcall__ snd_tune (const char* tune, unsigned char ch, unsigned tempo)
{
    const char* p = tune;
    const char* w;
    unsigned char n, i;
    unsigned beats, at = 0, t0;
    int note, d;

    if (!tempo) {
        return _directerrno (EINVAL);
    }
    for (w = word (&p, &n); w; w = word (&p, &n)) { /* (First, the whole tune checked) */
        if (!(n == 1 && *w == '-') && note_n (w, n) < 0) {
            return _directerrno (EINVAL);
        }
        if (!(w = word (&p, &n))) {
            return _directerrno (EINVAL);
        }
        for (beats = i = 0; i < n; ++i) {
            if (w[i] < '0' || w[i] > '9') {
                return _directerrno (EINVAL);
            }
            beats = beats * 10 + w[i] - '0';
        }
        if (!beats || beats > 255) {
            return _directerrno (EINVAL);
        }
    }
    if (snd_off (ch) < 0) {
        return -1;
    }
    p = tune;
    t0 = hy_ticks () + 2;
    for (;;) {
        d = (int) (t0 + (unsigned) ((unsigned long) at * (60 * HY_TICK_HZ) / tempo) - hy_ticks ());
        if (d > 0 && hy_sleep_ticks (d) < 0) {     /* (A note: the tune ends) */
            snd_off (ch);
            return -1;
        }
        snd_off (ch);
        if (!(w = word (&p, &n))) {
            return 0;
        }
        note = n == 1 && *w == '-' ? -1 : note_n (w, n);
        w = word (&p, &n);
        at += atoi (w);
        if (note >= 0 && snd_note (ch, note) < 0) {
            return -1;
        }
    }
}
