; ****************************************************************************
; hfs - HydraFS, the storage driver's second bank (its first: storage.s): #f, and the HydraFS commands and lines of
; #d's ctl files.  The format is docs/design/plans/HYDRAFS.md's, the old OS's, unchanged (versions 1 and 2, partitions,
; sparse files), and so is the code, ported from os_rom/fs/hfs_*.s: hfs/srv.s (the requests, walking, reading),
; hfs/write.s (the metadata buffer, the free map, growing and freeing, writing, create, remove, wstat),
; hfs/sparse.s (holes), hfs/format.s (format, label, partitions), hfs/check.s (the check, and the ctl file's
; lines).  Its state is here.  What it changed reaches the disk by the end of each request (HFS_FINISH).
;
; Its routines for storage.s (each through FAR2; dk = the disk, for those of a ctl file):
;   hfs_init        at the driver's start
;   hfs_serve       a request for #f (.A = R_*)
;   hfs_forget      disk dk is changing (started again, stopped): what's known of it forgotten, its open files
;                   let go of (their fids give E_BADF)
;   hfs_in_use      C = 1, .A = E_BUSY: a file on disk dk is open
;   hfs_ctl_lines   its lines in disk dk's ctl file (srv_text): its label, the space free, the last check's results
;   hfs_format, hfs_label, hfs_check            the ctl commands (their words in srv_argp: srvlib's)
;   hfs_format_ram  a RAM disk just started: a quick format, labelled RAM or SRAM

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"
.include "storage.inc"

; ---- The format (docs/design/plans/HYDRAFS.md)
HFS_BLOCK           = BLOCK     ; A block: a disk's sector
HFS_CLUSTER_BLOCKS  = 8         ; A cluster (what space is allocated in) on a card: 8 blocks, 4 KB (a volume's own:
                                ;   its superblock's HFS_SB_CSHIFT, HFS_SHIFT while it's the one in hand)
HFS_ENTRY_SIZE      = 64        ; A directory entry (a directory is a file of them)
HFS_NAME_MAX        = 31        ; A name's characters (any byte but '/' and 0; case-sensitive)
HFS_WC_N            = 64        ; The walk cache's records (srv.s): each its disk (1: $FF, free) ...
HFS_WC_DIR          = 1         ;   its directory's place (5: its entry's block and index, HFS_LOC) ...
HFS_WC_LEN          = 6         ;   its name's length (1) ...
HFS_WC_OK           = 7         ;   1: there; 0: not (1) ...
HFS_WC_LOC          = 8         ;   there: its entry's place (5) ...
HFS_WC_HASENT       = 13        ;   1: its entry kept (a directory's) (1) ...
HFS_WC_NAME         = 14        ;   its name (HFS_NAME_MAX) ...
HFS_WC_ENT          = HFS_WC_NAME + HFS_NAME_MAX        ;   and its entry (HFS_ENTRY_SIZE)
HFS_WC_SIZE         = HFS_WC_ENT + HFS_ENTRY_SIZE
HFS_VERSION         = 2         ; The newest format this reads (and writes, with a quick format): 2 has a
HFS_VERSION_FULL    = 1         ;   free map that's written as it's used (HFS_SB_MAPINIT); 1's is all written
HFS_CSHIFT          = 3         ;   and its cluster size, as a shift (HFS_CLUSTER_BLOCKS) ...
HFS_CSHIFT_RAM      = 1         ;   a RAM disk's (1 KB clusters: a small disk holds more files) ...
HFS_CSHIFT_MAX      = 3         ;   and the most this reads (1 to it: HFS_SHIFT_SET's tables)
HFS_PART_TYPE       = $7F       ; A HydraFS partition's type (for an OS in development, by convention)
HFS_PART_ALIGN      = 2048      ; A new partition starts on a 1 MB boundary (block 2048 on an empty card)
MBR_TABLE           = $1BE      ; Block 0: the partition table, 4 entries of MBR_ENTRY_SIZE bytes ...
MBR_ENTRY_SIZE      = 16
MBR_P_STATUS        = 0         ;   an entry: $00, or $80 (active)
MBR_P_CHS1          = 1         ;   its first block as cylinder, head, sector (3: $FE $FF $FF, LBA only)
MBR_P_TYPE          = 4         ;   its type (0: an unused entry)
MBR_P_CHS2          = 5         ;   its last block as cylinder, head, sector (3)
MBR_P_START         = 8         ;   its first block (4)
MBR_P_BLOCKS        = 12        ;   its size in blocks (4)
MBR_SIG             = $1FE      ;   and $55 $AA after the table
HFS_SB_MAGIC        = 0         ; The superblock (block 0): "HYDRAFS1"
HFS_SB_VERSION      = 8
HFS_SB_CSHIFT       = 9         ; The cluster size, as a shift
HFS_SB_CLUSTERS     = 12        ; Clusters in the data area
HFS_SB_MAP          = 16        ; The free map's first block
HFS_SB_MAPSZ        = 20        ;   its size in blocks (1 bit per cluster, 1 = in use)
HFS_SB_DATA         = 24        ; The data area's first block
HFS_SB_FREE         = 28        ; Free clusters (a hint: "check" recounts it)
HFS_SB_HINT         = 32        ; Where to look for free clusters next (a hint)
HFS_SB_NEXT_QID     = 36        ; The next qid id to give out
HFS_SB_STAMP        = 40        ; The latest modification stamp given (the clock's time, seconds since 2000)
HFS_SB_MAPINIT      = 44        ; Version 2: the free map's blocks written so far (from its first); the rest
                                ;   haven't been, and read as all free (version 1: ignored, all written)
HFS_SB_ROOT         = 64        ; The root directory's entry
HFS_SB_LABEL        = 128       ; The volume label, zero-terminated
HFS_E_NAME          = 0         ; A directory entry: its name, 32 bytes, zero-terminated (0 first: a free entry)
HFS_E_MODE          = 32        ; HFS_M_*
HFS_E_QVER          = 34        ; The qid's version, then its id
HFS_E_QID           = 36
HFS_E_SIZE          = 40        ; The size in bytes (up to 4 GB)
HFS_E_STAMP         = 44        ; The last change: the clock's time (seconds since 2000-01-01)
HFS_E_EXT1          = 48        ; Extent 1: its first cluster (4), its clusters (2)
HFS_E_EXT2          = 54        ; Extent 2 (0 clusters: unused)
HFS_E_EXTBLK        = 60        ; The first extent block, for the rest (0 = none)
HFS_EXT_SIZE        = 6
HFS_M_DIR           = $80       ; Mode: a directory
HFS_M_APPEND        = $40       ;   append-only
HFS_M_RO            = $01       ;   read-only
HFS_X_NEXT          = 0         ; An extent block: the next one (0 = the last) ...
HFS_X_COUNT         = 4         ;   its extents used (0-84) ...
HFS_X_FIRST         = 8         ;   and 84 extents, HFS_EXT_SIZE bytes each
HFS_X_MAX           = 84
HFS_HOLE            = $FF       ; A hole's extent: its first cluster is $FFFFFFFF
.assert     HFS_SB_ROOT / HFS_ENTRY_SIZE * HFS_ENTRY_SIZE = HFS_SB_ROOT, error, "The root's entry must be entry-aligned in block 0 (HFS_ENT_READ)"
.assert     HFS_X_FIRST + HFS_X_MAX * HFS_EXT_SIZE = HFS_BLOCK, error, "An extent block's extents fill it"
.assert     HFS_M_DIR = QT_DIR .and HFS_M_DIR = DM_DIR, error, "A directory's mode bit is its qid type and DM_DIR"

; ---- This server's
HFS_MAX_OPEN        = 32        ; Files open at once, all disks together (a fid is the slot: 0-31)
HFS_FID_DISKS       = $FE       ; The fid of the cards' directory (#f with no spec, its root)
HFS_DEPTH_MAX       = 16        ; Directories a walk goes down (HFS_STK, for REMOVE's directory)
HFS_PATH_MAX        = 80        ; A name with its spec before it (HFS_NAME)
HFS_IN_HOLE         = $FE       ; HFS_FILE_BLOCK's C = 1 .A for a block in a hole (not an error)
HFS_EOF             = E_EOF     ;   and for one past the file's clusters
HFS_MS_VALID        = $01       ; HFS_MSTATE: the metadata buffer holds HFS_MBLK ...
HFS_MS_FRESH        = $40       ;   a free map block that isn't on the disk yet: all free (HFS_MAP_BIT)
HFS_MS_DIRTY        = $80       ;   it's been changed, and not written yet
HFS_HF_WRITTEN      = $01       ; An open file's HFS_H_FLAGS: written since it was opened (a new qid version and
HFS_HF_DIRTY        = $80       ;   stamp); its entry (its size) changed since it was last written
HFS_FMT_FULL        = $01       ; HFS_FMT_OPT: the whole free map written (-f) ...
HFS_FMT_PART        = $02       ;   a partition made for it, if the card hasn't one (-p)
HFS_CK_WINDOW       = 65536     ; The check: the clusters a pass covers (8 KB of bitmap, 256 MB of disk) ...
HFS_CK_DEPTH_MAX    = 24        ;   the directories deep it walks ...
HFS_CK_LEVEL        = 68        ;   a directory in its walk: its entry, then where the walk is in it (4)
HFS_CK_BUF_SIZE     = HFS_CK_WINDOW / 8 + (HFS_CK_DEPTH_MAX + 1) * HFS_CK_LEVEL
.assert     HFS_WC_N * HFS_WC_SIZE <= HFS_CK_BUF_SIZE, error, "The walk cache's records: in the check's buffer"
.assert     HFS_WC_N <= 128, error, "The walk cache's records: looked at by .X, down to 0 (HFS_WC_SEARCH's bpl)"

; ---- The names the old code borrowed from the storage driver: its zero page, and the block buffer's
SD_LBA              = lba       ; A block (4)
SD_DEV              = dk        ; Its disk
SD_BUF              = bufp      ; The 512 bytes blk_read and blk_write move
SD_POS              = pos       ; A request's place in the file (4) ...
SD_DONE             = done      ;   the bytes it's moved (2) ...
SD_N                = n         ;   in this block (2)
SD_SRC              = src
SD_DST              = dst
SD_CVALID           = c_ok      ; (stz: blk holds nothing)

; Point ptr (zero page) at addr
.macro LOAD_ADDR addr, ptr
            LDR         ptr, addr
.endmacro

; One of the disk's superblock numbers (4 bytes at sb) from the superblock in the block buffer (V_FROM_SB), or to
; it at HFS_PTR (V_TO_SB): array + the disk * 4.  Modifies: .A, .X, .Y
.macro V_FROM_SB sb, array
            jsr         HFS_CARD_X
            ldy         #sb
:
            lda         (SD_CACHE),Y
            sta         array,X
            inx
            iny
            cpy         #sb + 4
            bne         :-
.endmacro

.macro V_TO_SB sb, array
            jsr         HFS_CARD_X
            ldy         #sb
:
            lda         array,X
            sta         (HFS_PTR),Y
            inx
            iny
            cpy         #sb + 4
            bne         :-
.endmacro

.zeropage
HFS_FP:     .res        2                                   ; The directory entry in play: HFS_ENT, or an open file's copy
HFS_XP:     .res        2                                   ; The extent being looked at (in the entry, or a block)
HFS_PTR:    .res        2                                   ; A pointer into a buffer
SD_CACHE:   .res        2                                   ; The block buffer (storage.s's blk)
HFS_NM:     .res        2                                   ; A name being walked (HFS_PATH), or a stat record's
HFS_WCP:    .res        2                                   ; A record of the walk cache (srv.s's HFS_WC_*)

.bss
; A request's
SD_LEFT:    .res        2                                   ; The bytes it has left to move
SD_OP:      .res        1                                   ; Its open mode (O_*)
SD_FID:     .res        1                                   ; Its fid
SD_TMP:     .res        1
HFS_CARD:   .res        1                                   ; The disk it's about
HFS_FID:    .res        1                                   ; An open file (0-31)
HFS_SPEC:   .res        1                                   ; <> 0: its mount had a spec (HFS_NAME)
HFS_PATH:   .res        HFS_PATH_MAX                        ; Its name, with the spec before it
HFS_CL:     .res        4                                   ; A cluster in the file, counted down through the extents
HFS_XBLK:   .res        4                                   ; The extent block being read
HFS_XCL:    .res        4                                   ; An extent: its first cluster ...
HFS_XLEN:   .res        2                                   ;   and its clusters (the 6 bytes in order)
HFS_SUB:    .res        1                                   ; The block wanted, inside its cluster (0-7)
HFS_OFS:    .res        2                                   ; An offset inside a block (0-511)
HFS_SKIP:   .res        2                                   ; A directory read: the records to skip (the offset's)
HFS_LEN:    .res        1                                   ; A name's length
HFS_DEPTH:  .res        1                                   ; A walk: how deep it is (HFS_STK) ...
HFS_ELEM:   .res        1                                   ;   and where the path element it's on starts
HFS_RQ:     .res        1                                   ; The request (R_*)
; The walk cache (srv.s): names looked up, and where their entries are, or that they aren't there (its records in
; the check's buffer, HFS_CK_BUF, while no check is running)
HFS_WC_H:   .res        HFS_WC_N                            ; Each record's hash (HFS_WC_KEY)
HFS_WC_NEXT: .res       1                                   ; The record a new name takes
HFS_WCD:    .res        1                                   ; The disk whose records are forgotten ($FF: all)
HFS_WCK:    .res        6                                   ; A lookup's: its disk and its directory's place ...
HFS_WCL:    .res        1                                   ;   its name's length ...
HFS_WCH:    .res        1                                   ;   its hash ...
HFS_WCI:    .res        1                                   ;   and the record looked at
.assert     HFS_XLEN = HFS_XCL + 4, error, "HFS_XCL and HFS_XLEN must be the extent's 6 bytes in order"
; The metadata buffer, the counters, allocating
HFS_META:   .res        2                                   ; The metadata buffer (HFS_MBUF): the free map, extent
                                                            ;   blocks, entries being written, the superblock;
                                                            ;   written back when another block is wanted, and at
                                                            ;   the end of the request
HFS_MBLK:   .res        4                                   ;   the block in it
HFS_MSTATE: .res        1                                   ;   HFS_MS_* bits
HFS_SBDIRTY: .res       1                                   ; <> 0: the disk's counters (HFS_V_FREE ...) changed
HFS_C:      .res        4                                   ; A cluster being allocated or freed
HFS_N4:     .res        4                                   ; Clusters left to look at; a count
HFS_D:      .res        4                                   ; A data cluster while its extent block is allocated
HFS_T4:     .res        4                                   ; A qid or stamp taken; a temporary
.assert     HFS_N4 = HFS_C + 4 .and HFS_D = HFS_C + 8 .and HFS_T4 = HFS_C + 12, error, "HFS_C, HFS_N4, HFS_D, HFS_T4: 4 bytes apart (HFS_SHR, HFS_PUT4)"
HFS_LASTB:  .res        4                                   ; A file's last extent: the extent block it's in (0: the
HFS_LASTO:  .res        2                                   ;   entry), and its offset there (0: the file has none)
HFS_DLOC:   .res        5                                   ; Create: the directory's place (block, index) ...
HFS_NLOC:   .res        5                                   ;   the new entry's (and where a directory scan found one)
HFS_PLOC:   .res        5                                   ; The place of the directory an entry being opened is in
HFS_LOC:    .res        5                                   ; Where an entry is: its block, then its index in it (0-7)
HFS_SCAN:   .res        1                                   ; HFS_DIR_SCAN: what it looks for (HFS_SCAN_*)
HFS_GREW:   .res        1                                   ; Create: <> 0: the directory got a new entry at its end
HFS_NAMEAT: .res        1                                   ; Create: where the new name starts in the name
HFS_PERM:   .res        1                                   ; Create: the new file's mode
HFS_RUN_VEC: .res       2                                   ; HFS_EACH_RUN: the routine for each run of clusters
HFS_ZBUF:   .res        2                                   ; A block of zeros, and the superblock, as a map block
                                                            ;   goes on the disk (HFS_MAP_WRITTEN; a hole's reads)
HFS_MW:     .res        4                                   ;   the map block being written
HFS_FMT_OPT: .res       1                                   ; Format: HFS_FMT_FULL, HFS_FMT_PART ...
HFS_FMT_BLKS: .res      4                                   ;   the size asked for, in blocks (0: the whole disk)
; Sparse files (sparse.s)
HFS_XBC:    .res        4                                   ; HFS_FILE_BLOCK: the extent block it's in (0: the entry)
HFS_HOLEP:  .res        2                                   ;   and a hole's extent it found (HFS_IN_HOLE)
HFS_POSB:   .res        4                                   ; A place in a file's extent list: the extent block (0:
HFS_POSO:   .res        2                                   ;   the entry), and the extent's offset there
HFS_POS_SIZE = 6
HFS_POSK:   .res        HFS_POS_SIZE                        ; A place, kept
HFS_XNEW:   .res        HFS_EXT_SIZE                        ; An extent being put in a file's list
HFS_XREP:   .res        HFS_EXT_SIZE                        ; Another, kept for later
HFS_HK:     .res        2                                   ; A hole being filled: the cluster's place in it ...
HFS_HN:     .res        2                                   ;   and its clusters
HFS_ENTCHG: .res        1                                   ; <> 0: the entry's extents changed (HFS_FILL writes it)
HFS_WZERO:  .res        1                                   ; $80: HFS_W_RANGE writes zeros, not the request's bytes
HFS_WPOS:   .res        4                                   ; A write past the end (HFS_EXTEND): its SD_POS, SD_LEFT
HFS_WLEFT:  .res        2                                   ;   and SD_DONE, kept while the gap is filled ...
HFS_WDONE:  .res        2
HFS_WK:     .res        4                                   ;   and the gap's whole clusters (a hole)
HFS_HOLEF:  .res        1                                   ; A read: <> 0: the block is in a hole (zeros)
HFS_KEEP:   .res        4                                   ; A file cut short (HFS_SHRINK): the clusters it keeps ...
HFS_TOTAL:  .res        4                                   ;   and those its extents have (before the last)
.assert     HFS_POSO = HFS_POSB + 4, error, "A place is HFS_POSB then HFS_POSO (HFS_POS_SIZE bytes)"
; The check (check.s)
HFS_CK_DEPTH: .res      1                                   ; How many directories deep its walk is
HFS_CK_MASK: .res       1                                   ; A cluster's bit; or the bits of a map byte that are
HFS_CK_N:   .res        3                                   ;   clusters.  Clusters of a run left to mark
HFS_CK_MARKS: .res      1                                   ; <> 0: the pass's bitmap has marks in it
HFS_CK_CARD: .res       1                                   ; The last check: its disk ($FF: none) ...
HFS_CK_FIXED: .res      1                                   ;   <> 0: it fixed the free map ("check fix")
HFS_CK_LOST: .res       4                                   ;   clusters marked in use that nothing uses ...
HFS_CK_UNMARKED: .res   4                                   ;   in use, but marked free ...
HFS_CK_TWICE: .res      4                                   ;   in use twice ...
HFS_CK_FREE: .res       4                                   ;   and free, as it counted them
HFS_CK_BUF: .res        2                                   ; Its buffer (PAGES_ALLOC at the start, or at the first
                                                            ;   check, then kept; the walk cache's when no check is
                                                            ;   running): a bitmap of HFS_CK_WINDOW clusters, then
HFS_CK_SP:  .res        2                                   ;   the walk's directories; the deepest, in it
.assert     HFS_CK_BUF - HFS_CK_LOST = 16, error, "The check's four counts: 4 bytes each, in a row (HFS_CK_ADD)"
; Each disk's (at the disk; the 4-byte ones at the disk * 4): whether it holds a HydraFS, where it starts, and the
; superblock's numbers the server works from.  The counters (free, hint, qid, stamp, the map written) change here,
; and go back to the superblock at the end of the request (HFS_FINISH)
HFS_V_STATE: .res       DISKS                               ; 0: not looked at yet; 1: HydraFS; $FF: not one
HFS_V_CSHIFT: .res      DISKS                               ; Its clusters' size, as a shift (its superblock's)
; The volume in hand's clusters (HFS_SHIFT_SET: HFS_VOLUME, HFS_FID_CHECK, a format): their size, as a shift ...
HFS_SHIFT:  .res        1
HFS_CBLK:   .res        1                                   ;   their blocks ...
HFS_CBMASK: .res        1                                   ;   less 1 ...
HFS_CBYTEHI: .res       1                                   ;   their bytes' high byte ...
HFS_CBYTEMASKHI: .res   1                                   ;   less 1
HFS_V_BASE: .res        DISKS * 4                           ; Its first block (its partition's, or 0)
HFS_V_CLUSTERS: .res    DISKS * 4                           ; Clusters in the data area
HFS_V_MAP:  .res        DISKS * 4                           ; The free map's first block
HFS_V_MAPSZ: .res       DISKS * 4                           ;   its size in blocks
HFS_V_DATA: .res        DISKS * 4                           ; The data area's first block
HFS_V_FREE: .res        DISKS * 4                           ; Free clusters
HFS_V_HINT: .res        DISKS * 4                           ; Where to look for a free cluster next
HFS_V_QID:  .res        DISKS * 4                           ; The next qid id
HFS_V_STAMP: .res       DISKS * 4                           ; The latest modification stamp
HFS_V_MINIT: .res       DISKS * 4                           ; The free map's blocks written (version 1: its size)
; The open files: each one's (at its fid; the blocks at the fid * 4), and a copy of its directory entry (at
; HFS_FILES + the fid * 64), so a read needs no block read to find its extents.  Every copy of one entry is kept the
; same (HFS_SYNC), and a walk takes an open file's copy over the disk's (its size may not be on the disk yet)
HFS_H_CARD: .res        HFS_MAX_OPEN                        ; Its disk, or $FF: a free slot
HFS_H_OMODE: .res       HFS_MAX_OPEN                        ; Its open mode (O_*)
HFS_H_REFS: .res        HFS_MAX_OPEN                        ; The fids on it (R_DUP, R_CLUNK)
HFS_H_FLAGS: .res       HFS_MAX_OPEN                        ; HFS_HF_* bits
HFS_H_EIDX: .res        HFS_MAX_OPEN                        ; Where its entry is: the entry's index in its block ...
HFS_H_EBLK: .res        HFS_MAX_OPEN * 4                    ;   and the block
HFS_H_PIDX: .res        HFS_MAX_OPEN                        ; Where its directory's entry is (for a rename; the root:
HFS_H_PBLK: .res        HFS_MAX_OPEN * 4                    ;   its own place)
HFS_FILES:  .res        HFS_MAX_OPEN * HFS_ENTRY_SIZE
HFS_ENT:    .res        HFS_ENTRY_SIZE                      ; The entry a walk is at
HFS_STAT:   .res        SR_SIZE                             ; A stat record; a label; a line
HFS_STK:    .res        HFS_DEPTH_MAX * 5                   ; A walk's directories (their places)
HFS_NEWE:   .res        HFS_ENTRY_SIZE                      ; A new entry being made; an entry's old extents
HFS_MBUF:   .res        HFS_BLOCK                           ; The metadata buffer's 512 bytes ...
HFS_ZEROS:  .res        HFS_BLOCK                           ;   and HFS_ZBUF's
.assert     HFS_MAX_OPEN * 4 <= 256, error, "An open file's blocks: at the fid * 4, in .X"

.segment "CODE2"
; ****************************************************************************
; The driver's start: no disk looked at, no file open, no check; the buffers (the check's, the walk cache's too, from
; this task's pages now: none, and the cache is off, and the check takes them at its first)
hfs_init:
            LDR         SD_CACHE, blk
            LDR         HFS_META, HFS_MBUF
            LDR         HFS_ZBUF, HFS_ZEROS
            stz         HFS_MSTATE
            stz         HFS_SBDIRTY
            stz         HFS_WZERO
            lda         #$FF
            sta         HFS_CK_CARD
            ldx         #HFS_MAX_OPEN - 1
:
            sta         HFS_H_CARD,X
            dex
            bpl         :-
            lda         #>(HFS_CK_BUF_SIZE + 255)
            jsr         PAGES_ALLOC
            bcs         :+
            MOVR        HFS_CK_BUF, r0
:
            stz         HFS_WC_NEXT
            jsr         HFS_WC_CLEAR
            clc
            rts

.include "hfs/srv.s"
.include "hfs/write.s"
.include "hfs/sparse.s"
.include "hfs/format.s"
.include "hfs/check.s"
