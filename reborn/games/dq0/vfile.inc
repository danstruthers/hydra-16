/*
** vfile.inc - Dingus Quest Zero's region files' versions (world.h's struct vfh: DUNGEON.md's no fixed size), every
** task's that writes or reads them compiles the same (the world's, in each of its swapped parts that wants them:
** ../dq0w/ipage.c, wfile.c; the maker's; the denizens'): a store's region kept in its file in the save folder, in
** versions, each written whole over a slot that's neither the last save's nor (with three slots) the newest, its
** header (its stamp) written last, so a version cut short by a crash is no version; read, the newest valid version:
** one the last save had (stamped up to the hub's vsaved), or this session's (past its vbase), never one a session wrote
** and ended without its save (stamped between them).  A stamp's past the hub's vlo, this task's count from it (each
** writer's own: two tasks' stamps may be the same, never two versions of one file's).  A version 8 save's change file
** (struct chead's header, CH_MAGIC: its slot ".dat") is stamped 0, the save's.
**
** Before it's included: VF_STATIC (static, for a swapped part's own copy; or nothing), and the code's segments (a
** swapped part's: its BSS too, the part's own, what's kept between calls the main RAM's).  The hub's header in the
** window after each call.
*/

#pragma bss-name (push, "BSS")
static unsigned long vlast;                     /* (The hub's vlo this task last stamped from, and its count since) */
static unsigned vcount;
#pragma bss-name (pop)
static struct vfh vh, vb;                       /* (A version's header, as read; the newest's) */
static unsigned long vst[3];                    /* (Each slot's stamp, as vf_scan found it, if it's valid) */
static unsigned long vsb[2], vstamp;           /* (The hub's vsaved and vbase, as a call began; vf_make's stamp) */
#define vsv             vsb[0]
#define vbs             vsb[1]
static unsigned char vsave, vnew;               /* (vf_scan's: the save's version's slot, the newest's; 3 none) */
static int vfd;
static const char vhex[] = "0123456789abcdef";
static const char vslot[] = "t12";              /* (Slots 0, 1, 2: ".dat", ".da1", ".da2") */
static char* vpath;

/* a < b (stamps: 4 bytes, the low first) */
static unsigned char __fastcall__ vlt (const unsigned long* a, const unsigned long* b)
{
    static const unsigned char* p;
    static const unsigned char* q;
    static unsigned char i;

    p = (const unsigned char*) a;
    q = (const unsigned char*) b;
    for (i = 4; i--; ) {
        if (p[i] != q[i]) {
            return p[i] < q[i];
        }
    }
    return 0;
}

/* Slot k's version of vpath (its last letter made k's) opened, its header read (vh): its fd past it, or -1 (none, or
** not valid: not this game's, or a session's that ended without its save) */
static int __fastcall__ vf_head (unsigned char k)
{
    unsigned gen;

    vpath[strlen (vpath) - 1] = vslot[k];
    if ((vfd = open (vpath, O_RDONLY)) < 0) {
        return -1;
    }
    hub_map (WORLD_BANK);
    gen = WREC->cgen;
    hub_map (0);
    vh.stamp = 0;
    if (read (vfd, &vh, sizeof (struct chead)) == sizeof (struct chead) && vh.gen == gen) {
        if (vh.magic == CH_MAGIC && !k) {
            return vfd;                         /* (A version 8 save's: stamped 0) */
        }
        if (vh.magic == VF_MAGIC && read (vfd, &vh.stamp, 4) == 4 && (!vlt (&vsv, &vh.stamp) || vlt (&vbs, &vh.stamp))) {
            return vfd;
        }
    }
    close (vfd);
    return -1;
}

/* path's slots' versions looked at (each one's stamp in vst, the save's version and the newest: vsave, vnew), the
** newest left open (its fd back, its header in vh), or -1 */
static int __fastcall__ vf_scan (char* path, unsigned char slots)
{
    unsigned char k;
    int fd = -1;

    vpath = path;
    hub_map (0);
    memcpy (vsb, (void*) &HUB->vsaved, 8);      /* (vsaved, vbase) */
    vsave = vnew = 3;
    for (k = 0; k < slots; ++k) {
        if (vf_head (k) < 0) {
            continue;
        }
        vst[k] = vh.stamp;
        if (!vlt (&vsv, &vh.stamp) && (vsave == 3 || vlt (&vst[vsave], &vh.stamp))) {
            vsave = k;
        }
        if (vnew == 3 || vlt (&vst[vnew], &vh.stamp)) {
            vnew = k;
            if (fd >= 0) {
                close (fd);
            }
            fd = vfd;
            memcpy (&vb, &vh, sizeof vh);
        } else {
            close (vfd);
        }
    }
    memcpy (&vh, &vb, sizeof vh);               /* (The newest's header: its fd past it) */
    return fd;
}

/* path's newest valid version: its fd past its header (in h), or -1 */
VF_STATIC int __fastcall__ vf_open (char* path, unsigned char slots, struct vfh* h)
{
    int fd = vf_scan (path, slots);

    memcpy (h, &vh, sizeof vh);
    return fd;
}

/* A new version of path begun, over a slot that's neither the save's version (the newest stamped up to the hub's
** vsaved) nor the newest (else, two slots, over the newest): its fd, past the room for its header, or -1 (the hub's
** vhi 0: the game not begun) */
VF_STATIC int __fastcall__ vf_make (char* path, unsigned char slots)
{
    unsigned char k;
    int fd = vf_scan (path, slots);

    if (fd >= 0) {
        close (fd);
    }
    for (k = 0; k == vsave || k == vnew; ++k) {
    }
    if (k >= slots) {
        k = !vsave;
    }
    hub_map (0);
    if (!HUB->vhi) {
        return -1;
    }
    if (memcmp (&vlast, (void*) &HUB->vlo, 4)) {
        memcpy (&vlast, (void*) &HUB->vlo, 4);
        vcount = 0;
    }
    vstamp = vlast + ++vcount;
    path[strlen (path) - 1] = vslot[k];
    if ((vfd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        vh.magic = 0;                           /* (No version till its header's written: vf_done's) */
        write (vfd, &vh, sizeof vh);
    }
    return vfd;
}

/* Its header written (n records, its stamp), the version closed */
VF_STATIC void __fastcall__ vf_done (int fd, unsigned n)
{
    hub_map (WORLD_BANK);
    vh.gen = WREC->cgen;
    hub_map (0);
    vh.magic = VF_MAGIC;
    vh.n = n;
    vh.stamp = vstamp;
    lseek (fd, 0, SEEK_SET);
    write (fd, &vh, sizeof vh);
    close (fd);
}

/* path made the save folder's file of a store's region (pre, its two letters: "rg" ...; preSXXYY.dat, a stratum past
** 15's preSSXXYY.dat), its slot ".dat": 0, no folder (the hub's savedir "") */
static char* __fastcall__ vf_hex (char* p, unsigned char b)
{
    *p = vhex[b >> 4];
    p[1] = vhex[b & 15];
    return p + 2;
}

VF_STATIC unsigned char __fastcall__ vf_name (char* path, const char* pre, unsigned char sn, unsigned char rx,
    unsigned char ry)
{
    char* p;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return 0;
    }
    strcpy (path, HUB->savedir);
    p = path + strlen (path);
    p[0] = '/';
    p[1] = pre[0];
    p[2] = pre[1];
    p = sn > 15 ? vf_hex (p + 3, sn) : (p[3] = vhex[sn], p + 4);
    strcpy (vf_hex (vf_hex (p, rx), ry), ".dat");
    return 1;
}
