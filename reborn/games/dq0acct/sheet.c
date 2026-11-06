/*
** sheet.c - (dq0acct, overlay 3) the sheet read and written (the library's harnfile.c compiled here, as the referee's is), the file's
** other lines kept as they were, in this overlay's bank.
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")
#pragma bss-name ("OV3BSS")
#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0harn/dq0harn.h"
#include "dq0acct.h"

static char own[2000];                          /* (The sheet's other lines, each 0-ended: put back as they were) */
static unsigned ownn;
#pragma bss-name ("OV3RODATA")

static const char* sheet_path (void)
{
    strcpy (path, top);
    strcat (path, "/pc.hm");
    return path;
}


static unsigned char __fastcall__ hexd (char c)
{
    return c <= '9' ? c - '0' : c - 'a' + 10;
}

static void __fastcall__ own_line (char* l)
{
    unsigned char k;
    unsigned v[11];
    char* p;

    if (!strncmp (l, "dealt ", 6)) {
        for (k = 0; k < HARN_INJMAX && l[k + 6]; ++k) {
            pc_by[k] = l[k + 6] & 7;
        }
        return;
    }
    if (!strncmp (l, "body ", 5) && l[5] != 'u') {
        l += 5;
        memset (v, 0, sizeof v);
        for (k = 0; l && k < 11; ++k) {
            v[k] = atoi (l);
            if ((l = strchr (l, ' ')) != 0) {
                ++l;
            }
        }
        pv.hr = v[0];
        pv.loc = v[1];
        pv.kind = v[2];
        pv.left = v[3];
        pd.hr = v[5];
        pd.kind = v[6];
        pd.left = v[7];
        pd.inc = v[8];
        afffl = v[10];
        return;
    }
    if (!strncmp (l, "body u", 6)) {
        for (p = strchr (l + 5, ' '); p && p[1] > ' '; p += 4) {
            k = hexd (p[1]) << 4 | hexd (p[2]);
            if (k < HARN_NSKILLS) {
                used[k] = 5 * hexd (p[3]) + hexd (p[4]);
            }
        }
        /* (The line itself kept: its rolls stand) */
    }
    if (!strncmp (l, "language ", 9) || !strncmp (l, "script ", 7)) {
        if (ntongues < NTONGUES) {              /* (Its tongues and scripts: practised, skill.c's; written back after the rest) */
            strncpy (tongue[ntongues++], l, sizeof tongue[0] - 1);
        }
        return;
    }
    if (!strncmp (l, "name ", 5)) {
        strncpy (who, l + 5, sizeof who - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (calling, l + 11, sizeof calling - 1);
    } else if (!strncmp (l, "pace ", 5) && strlen (l) >= 10) {
        l[9] = '0';                             /* (Its exertion gone: "pace 2 1 0") */
        l[10] = 0;
    }
    k = strlen (l) + 1;
    if (ownn + k <= sizeof own) {
        memcpy (own + ownn, l, k);
        ownn += k;
    }
}

unsigned char o_sheet_read (void)
{
    int fd;
    unsigned char ok;

    ownn = ntongues = 0;
    if ((fd = open (sheet_path (), O_RDONLY)) < 0) {
        return 0;
    }
    ok = !harn_read (fd, &pc, own_line);
    close (fd);
    return ok;
}

void o_sheet_write (void)
{
    static char line[64];
    char b[8];
    unsigned v[11];
    unsigned i;
    unsigned char k;
    int fd;

    if ((fd = open (sheet_path (), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    harn_write (fd, &pc);
    for (i = 0; i < ownn; i += strlen (own + i) + 1) {
        harn_write_line (fd, own + i);
    }
    memset (v, 0, sizeof v);
    v[0] = pv.hr;
    v[1] = pv.loc;
    v[2] = pv.kind;
    v[3] = pv.left;
    v[5] = pd.hr;
    v[6] = pd.kind;
    v[7] = pd.left;
    v[8] = pd.inc;
    v[10] = afffl;
    strcpy (line, "body");
    for (k = 0; k < 11; ++k) {
        strcat (line, " ");
        strcat (line, utoa (v[k], b, 10));
    }
    harn_write_line (fd, line);
    strcpy (line, "dealt ");
    for (k = 0; k < pc.ninj; ++k) {
        line[k + 6] = '0' | pc_by[k];
    }
    line[k + 6] = 0;
    harn_write_line (fd, line);
    for (k = 0; k < ntongues; ++k) {
        harn_write_line (fd, tongue[k]);
    }
    close (fd);
}

