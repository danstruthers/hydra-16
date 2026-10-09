/*
** hydracalls.h - the Hydra-16's system calls, error codes and constants, for C (cc65).  Made by tools/apigen.js
** from spec/: don't edit.  hydra.h includes it.  A call's name is its slot in the jump table, for hy_call; the
** registers are in spec/api.def and the reference (/rom/doc/api.md).  Each name has HY_ before it (cc65's
** headers have some of them: O_RDWR, say).
*/

#ifndef _HYDRACALLS_H
#define _HYDRACALLS_H

/* ---- system: System information and services */
#define HY_SYSINFO              0xF800
#define HY_ERRSTR               0xF803
#define HY_KMESG                0xF806
#define HY_REBOOT               0xF809
#define HY_XCALL                0xF80C

/* ---- task: Tasks: starting and ending them, waiting, scheduling, ticks */
#define HY_EXITS                0xF830
#define HY_WAIT                 0xF833
#define HY_SPAWN                0xF836
#define HY_GETPID               0xF839
#define HY_YIELD                0xF83C
#define HY_SLEEP                0xF83F
#define HY_SLEEP_UNTIL          0xF842
#define HY_TICKS                0xF845
#define HY_PREEMPT_OFF          0xF848
#define HY_PREEMPT_ON           0xF84B
#define HY_TASKINFO             0xF84E
#define HY_NOTIFY               0xF851
#define HY_NOTE                 0xF854
#define HY_MODINFO              0xF857
#define HY_ENV_GET              0xF85A
#define HY_ENV_PUT              0xF85D
#define HY_ENV_DEL              0xF860
#define HY_ENV_NAME             0xF863
#define HY_TASKREAD             0xF866
#define HY_GETPPID              0xF869
#define HY_SEM_NEW              0xF86C
#define HY_SEM_ACQUIRE          0xF86F
#define HY_SEM_TRY              0xF872
#define HY_SEM_RELEASE          0xF875
#define HY_SEM_FREE             0xF878

/* ---- memory: Memory: the break, pages, banks, shared segments (phase 1.8) */
#define HY_BREAK                0xF890
#define HY_PAGES_ALLOC          0xF893
#define HY_PAGES_FREE           0xF896
#define HY_BANKS                0xF899
#define HY_BANKS_ALLOC          0xF89C
#define HY_BANKS_FREE           0xF89F
#define HY_SEG_CREATE           0xF8A2
#define HY_SEG_ATTACH           0xF8A5
#define HY_SEG_DETACH           0xF8A8
#define HY_SEG_MAP              0xF8AB
#define HY_SEGINFO              0xF8AE
#define HY_BANKS_ALLOC_IN       0xF8B1
#define HY_SEG_CREATE_IN        0xF8B4

/* ---- file: Files (phase 2) */
#define HY_OPEN                 0xF8C0
#define HY_CREATE               0xF8C3
#define HY_CLOSE                0xF8C6
#define HY_READ                 0xF8C9
#define HY_WRITE                0xF8CC
#define HY_SEEK                 0xF8CF
#define HY_STAT                 0xF8D2
#define HY_FSTAT                0xF8D5
#define HY_WSTAT                0xF8D8
#define HY_FWSTAT               0xF8DB
#define HY_REMOVE               0xF8DE
#define HY_DUP                  0xF8E1
#define HY_DUP2                 0xF8E4
#define HY_CHDIR                0xF8E7
#define HY_GETCWD               0xF8EA
#define HY_PIPE                 0xF8ED
#define HY_FD2PATH              0xF8F0

/* ---- name: Namespaces (phase 2) */
#define HY_BIND                 0xF920
#define HY_MOUNT                0xF923
#define HY_UNMOUNT              0xF926
#define HY_NSINFO               0xF929

/* ---- cons: Console helpers: a byte or a string to stdout, a byte from stdin */
#define HY_PUTC                 0xF950
#define HY_PUTS                 0xF953
#define HY_GETC                 0xF956
#define HY_PUTHEX               0xF959

/* ---- time: The clock (phase 5) */
#define HY_TIME                 0xF980
#define HY_TIME_SET             0xF983
#define HY_RTC                  0xF986

/* ---- server: Servers and drivers */
#define HY_IRQ_OWN              0xF9B0
#define HY_IRQ_RELEASE          0xF9B3
#define HY_WAKE                 0xF9B6
#define HY_PAUSE                0xF9B9
#define HY_NOTE_POST            0xF9BC
#define HY_SRV_REGISTER         0xF9BF
#define HY_SRV_TAKE             0xF9C2
#define HY_SRV_REPLY            0xF9C5
#define HY_CLIENT_READ          0xF9C8
#define HY_CLIENT_WRITE         0xF9CB
#define HY_NOTE_QUEUE           0xF9CE
#define HY_ROMREAD              0xF9D1
#define HY_TASKMEM              0xF9D4
#define HY_TASKSTOP             0xF9D7
#define HY_TASKSTEP             0xF9DA

/* ---- dbg: Debugging and tests: unstable, may change in any version */
#define HY_DBG_SCALL            0xFA10
#define HY_DBG_KCOPY            0xFA13
#define HY_DBG_PS               0xFA16

/* ---- error codes (_oserror) */
#define HY_E_PERM               0x1       /* not allowed */
#define HY_E_INVAL              0x2       /* invalid argument */
#define HY_E_NOSYS              0x3       /* no such call */
#define HY_E_AGAIN              0x4       /* not yet: try again */
#define HY_E_INTR               0x5       /* interrupted */
#define HY_E_NOMEM              0x6       /* out of memory */
#define HY_E_BUSY               0x7       /* busy */
#define HY_E_RANGE              0x8       /* out of range */
#define HY_E_FAULT              0x9       /* bad address */
#define HY_E_NAMETOOLONG        0xA       /* name too long */
#define HY_E_TOOBIG             0xB       /* too big */
#define HY_E_NOENT              0x20      /* not found */
#define HY_E_EXIST              0x21      /* already exists */
#define HY_E_NOTDIR             0x22      /* not a directory */
#define HY_E_ISDIR              0x23      /* is a directory */
#define HY_E_NOTEMPTY           0x24      /* directory not empty */
#define HY_E_BADF               0x25      /* bad file descriptor */
#define HY_E_MFILE              0x26      /* no free file descriptor */
#define HY_E_NFILE              0x27      /* too many open files */
#define HY_E_NOSPC              0x28      /* disk full */
#define HY_E_ROFS               0x29      /* read-only */
#define HY_E_IO                 0x2A      /* i/o error */
#define HY_E_NODEV              0x2B      /* no such device */
#define HY_E_PIPE               0x2C      /* broken pipe */
#define HY_E_NOEXEC             0x2D      /* not a program */
#define HY_E_EOF                0x2E      /* end of file */
#define HY_E_NOTFS              0x2F      /* no file system */
#define HY_E_MEDIA              0x30      /* medium error */
#define HY_E_NOTASK             0x40      /* no free task */
#define HY_E_SRCH               0x41      /* no such task */
#define HY_E_CHILD              0x42      /* no such child */
#define HY_E_NSFULL             0x50      /* namespace full */
#define HY_E_NSLOOP             0x51      /* too many binds */

/* ---- constants */
#define HY_ABI_VERSION          1         /* The ABI version this include describes */
#define HY_TICK_HZ              200       /* Ticks a second */
#define HY_INIT_TASK            1         /* init's task (and its note group: the console's foreground at first) */
#define HY_PROG_ZP              0x22      /* A program's own zero page: $22-$7F */
#define HY_PROG_ZP_END          0x80
#define HY_TASK_ARGS            0x0350    /* A task's arguments: zero-terminated strings, an empty one after the last (176 bytes at most) */
#define HY_ARGS_MAX             176       /* SPAWN's arguments: their bytes at most (each string's 0, and the empty one that ends them) */
#define HY_TASK_NAME            0x0230    /* A task's name, zero-terminated */
#define HY_IRQ_RESCHED          0x80      /* An irq entry's answer: a task switch, please */
#define HY_LINE_VIA_T2          16        /* IRQ_OWN's line for VIA timer 2 (the hardware's lines: hw.inc) */
#define HY_LINE_VIA_CA1         17        /* IRQ_OWN's line for VIA CA1 (J27's pin 11): owning it turns CA1's interrupt on; the owner clears its flag */
#define HY_TASK_EVENT           0xBD      /* A task's event count: a server adds 1 (inc TASK_EVENT) when what its clients wait for has happened */
#define HY_HT_PROGRAM           1         /* HYX2 module types */
#define HY_HT_DRIVER            2
#define HY_HT_LIBRARY           3
#define HY_HF_INPLACE           0x01      /* HYX2 flags: runs in place in the paged ROM */
#define HY_HF_BOOT              0x02      /* HYX2 flags: a driver the kernel starts at boot */
#define HY_HX_MAGIC             0         /* The HYX2 header (HX_SIZE bytes, a program's first): HYX2 */
#define HY_HX_HSIZE             4         /* HYX2: the header's size (48) */
#define HY_HX_TYPE              5         /* HYX2: the type (HT_*) */
#define HY_HX_FLAGS             6         /* HYX2: the flags (HF_*) */
#define HY_HX_ABI               7         /* HYX2: the ABI version it was built for */
#define HY_HX_LOAD              8         /* HYX2: its load address (2: $A000 in place, $0800 a RAM program) */
#define HY_HX_LENGTH            10        /* HYX2: its image's length (2: in its last bank, in place) */
#define HY_HX_DATA_LOAD         12        /* HYX2: its data, where it is in the image (2) ... */
#define HY_HX_DATA_RUN          14        /* HYX2: ... where it runs (2) ... */
#define HY_HX_DATA_LEN          16        /* HYX2: ... and its length (2) */
#define HY_HX_BSS               18        /* HYX2: its BSS (2) ... */
#define HY_HX_BSS_LEN           20        /* HYX2: ... and its length (2: cleared as it starts) */
#define HY_HX_TOP               22        /* HYX2: the top of the RAM it uses (2: its break starts there) */
#define HY_HX_MAIN              24        /* HYX2: entry: main (a program), init (a driver) */
#define HY_HX_SERVE             26        /* HYX2: entry: serve (a driver) */
#define HY_HX_IRQ               28        /* HYX2: entry: irq (a driver) */
#define HY_HX_STOP              30        /* HYX2: entry: stop (a driver) */
#define HY_HX_DEVICE            32        /* HYX2: its device letter (a driver) */
#define HY_HX_BANKS             33        /* HYX2: the banks it spans (in place) */
#define HY_HX_VERSION           34        /* HYX2: its version (2) */
#define HY_HX_NAME              36        /* HYX2: its name (12: zero-terminated) */
#define HY_HX_SIZE              48
#define HY_ME_BANK              0         /* MODINFO's answer (ME_SIZE bytes): the module's first paged ROM bank */
#define HY_ME_BANKS             1         /* MODINFO: the banks it spans */
#define HY_ME_TYPE              2         /* MODINFO: its type (HT_*) */
#define HY_ME_FLAGS             3         /* MODINFO: its flags (HF_*) */
#define HY_ME_NAME              4         /* MODINFO: its name (12 bytes, zero-padded) */
#define HY_ME_SIZE              16
#define HY_TI_STATE             0         /* TASKINFO's answer: the task's state (0: free, 1 ready, 2 wait, 3 call, 4 idle, 5 new, 6 sleep, 7 blocked, 8 event) */
#define HY_TI_FLAGS             1         /* TASKINFO: its flags (TF_DRIVER, TF_STOPPED) */
#define HY_TF_DRIVER            0x01      /* TI_FLAGS: a driver */
#define HY_TF_STOPPED           0x80      /* TI_FLAGS: stopped (/proc's ctl: TASKSTOP) */
#define HY_TI_PARENT            2         /* TASKINFO: its parent ($FF: none) */
#define HY_TI_CPU               3         /* TASKINFO: its CPU time, in ticks (3 bytes) */
#define HY_TI_BANK              6         /* TASKINFO: its module's paged ROM bank ($FF: none) */
#define HY_TI_TYPE              7         /* TASKINFO: its module's type (HT_*; 0: none) */
#define HY_TI_NAME              8         /* TASKINFO: its name, zero-terminated (16 bytes) */
#define HY_TI_GROUP             24        /* TASKINFO: its note group */
#define HY_TI_SIZE              25        /* TASKINFO: the answer's size */
#define HY_NOTE_INTERRUPT       1         /* Notes: Ctrl-C (default: the end, 130) */
#define HY_NOTE_KILL            2         /* Notes: kill, never caught (137) */
#define HY_NOTE_HANGUP          3         /* Notes: hangup (129) */
#define HY_NOTE_ALARM           4         /* Notes: alarm (142) */
#define HY_NOTE_BRK             5         /* Notes: sys: brk, a BRK instruction (133) */
#define HY_NOTE_USER            16        /* Notes: 16-31 are the programs' own (128 + the note) */
#define HY_NOTE_GROUP           0x80      /* NOTE's .A: a note group, not a task */
#define HY_SEM_MUTEX            0x01      /* SEM_NEW's .X: a mutex (its count 1, given back only by its holder) */
#define HY_REBOOT_HWTEST        0x01      /* REBOOT's .A: into the hardware test */
#define HY_SEM_MAX              16        /* Semaphores: as many as there can be at once */
#define HY_SPAWN_NEWGROUP       0x01      /* SPAWN's flags: the child starts a note group of its own */
#define HY_SPAWN_NEWNS          0x02      /* SPAWN's flags: the child starts with an empty namespace of its own (else it shares its parent's) */
#define HY_SPAWN_FDMAP          0x04      /* SPAWN's flags: the child's fds are r2's map (else the caller's 0, 1 and 2) */
#define HY_SPAWN_NOENV          0x08      /* SPAWN's flags: the child starts with an empty environment (else a copy of its parent's) */
#define HY_SPAWN_STOPPED        0x10      /* SPAWN's flags: the child stops at its entry point, for the debugger (/proc/N/ctl's step) */
#define HY_ENV_SIZE             1024      /* TASKREAD's TR_ENV: its buffer, an environment's first variables, whole, and a 0 */
#define HY_ENV_MAX              8192      /* An environment's bytes at most: each variable its name's length, its name, its value's length (2), its value; a 0 */
#define HY_ENV_NAME_MAX         31        /* A variable's name's length at most */
#define HY_SPAWN_FDS            15        /* SPAWN's fd map: the child's fds it can give, at most (its fd 15 is its loader's as it starts) */
#define HY_MREPL                0         /* BIND's and MOUNT's flags: in place of what's at old */
#define HY_MBEFORE              1         /* BIND's and MOUNT's flags: a union, new first */
#define HY_MAFTER               2         /* BIND's and MOUNT's flags: a union, new last */
#define HY_MCREATE              4         /* BIND's and MOUNT's flags: a CREATE in the union comes to new */
#define HY_EXIT_MSG_MAX         31        /* An exit message's length at most (WAIT's buffer: 32 bytes) */
#define HY_KMESG_SIZE           4096      /* KMESG: the kernel's messages held at most (then the oldest go) */
#define HY_TR_ARGS              0         /* TASKREAD: a task's arguments (ARGS_MAX bytes) */
#define HY_TR_CWD               1         /* TASKREAD: a task's current directory (PATH_MAX + 1 bytes) */
#define HY_TR_ENV               2         /* TASKREAD: a task's environment's first variables, whole, and a 0 (ENV_SIZE bytes at most) */
#define HY_TR_FRAME             3         /* TASKREAD: a task's state and frame (TF_SIZE bytes) */
#define HY_TR_ENVAT             4         /* TASKREAD: part of a task's environment (r1 bytes at most, from offset r2; .A/.X its bytes in use) */
#define HY_TR_FD                5         /* TASKREAD: a task's fd r2 (FI_SIZE bytes) */
#define HY_FI_NAME              0         /* TR_FD's: the name its file was opened by (PATH_MAX + 1 bytes, zero-terminated) ... */
#define HY_FI_MODE              64        /* TR_FD's: its open mode (O_*) ... */
#define HY_FI_DEV               65        /* TR_FD's: its server's device letter ... */
#define HY_FI_QTYPE             66        /* TR_FD's: its qid type (QT_DIR ...) ... */
#define HY_FI_OFFSET            67        /* TR_FD's: and its offset (4 bytes, low first) */
#define HY_FI_SIZE              71
#define HY_TF_STATE             0         /* TR_FRAME's: the task's state (0 free, 1 ready, 2 waiting, 3 calling, 4 idle, 5 new, 6 sleeping, 7 blocked, 8 waiting for an event) */
#define HY_TF_S                 1         /* TR_FRAME's: its stack pointer, as it was before the frame */
#define HY_TF_U                 2         /* TR_FRAME's: the frame: U */
#define HY_TF_Y                 3
#define HY_TF_W                 4
#define HY_TF_X                 5
#define HY_TF_A                 6
#define HY_TF_P                 7
#define HY_TF_PC                8         /* TR_FRAME's: PC (2) */
#define HY_TF_RAM               10        /* TR_FRAME's: its RAM bank register ($00) */
#define HY_TF_ROM               11        /* TR_FRAME's: its paged ROM bank register ($01) */
#define HY_TF_SIZE              12
#define HY_TM_READ              0         /* TASKMEM: from the task's memory */
#define HY_TM_WRITE             0x01      /* TASKMEM: to the task's memory */
#define HY_TM_BANK              0x80      /* TASKMEM's flag: its RAM bank r3, at $8000-$9FFF */
#define HY_TS_STEP              0         /* TASKSTEP: one instruction */
#define HY_TS_NEXT              1         /* TASKSTEP: one instruction, a JSR's subroutine whole */
#define HY_TS_BREAKS            2         /* TASKSTEP: from now, the task's BRKs stop it (the debugger's breakpoints) */
#define HY_TS_NOBREAKS          3         /* TASKSTEP: its BRKs are its own again (NOTE_BRK) */
#define HY_NI_FROM              0         /* NSINFO's answer (NI_SIZE bytes): the mount point (PATH_MAX + 1 bytes, zero-terminated) */
#define HY_NI_PATH              64        /* NSINFO: the path in the device (zero-terminated, no leading /) */
#define HY_NI_DEV               128       /* NSINFO: the device letter */
#define HY_NI_SPEC              129       /* NSINFO: the spec (8 bytes, zero-padded, and a 0) */
#define HY_NI_FLAGS             138       /* NSINFO: the flags (MCREATE) */
#define HY_NI_SEQ               139       /* NSINFO: its place among the union's members (lowest first) */
#define HY_NI_SIZE              140
#define HY_KEY_UP               0x80      /* Keys: a raw console read gives the terminal's cursor and function keys as one code each */
#define HY_KEY_DOWN             0x81
#define HY_KEY_RIGHT            0x82
#define HY_KEY_LEFT             0x83
#define HY_KEY_HOME             0x84
#define HY_KEY_END              0x85
#define HY_KEY_INS              0x86
#define HY_KEY_DEL              0x87
#define HY_KEY_PGUP             0x88
#define HY_KEY_PGDN             0x89
#define HY_KEY_F1               0x8A      /* Keys: F1-F12 are $8A-$95 */
#define HY_KEY_F2               0x8B
#define HY_KEY_F3               0x8C
#define HY_KEY_F4               0x8D
#define HY_KEY_F5               0x8E
#define HY_KEY_F6               0x8F
#define HY_KEY_F7               0x90
#define HY_KEY_F8               0x91
#define HY_KEY_F9               0x92
#define HY_KEY_F10              0x93
#define HY_KEY_F11              0x94
#define HY_KEY_F12              0x95
#define HY_KEY_FOCUS            0x97      /* Keys: the focus moved to another window of this one's group; its number is the next key (a raw read's, keys hydra's) */
#define HY_KEY_RESIZE           0x96      /* Keys: the window's size changed (a raw read's, keys hydra's: consctl reads with the new one) */
#define HY_KEY_MOD              0x98      /* Keys: a key the terminal sent modified (keys mods's): the modifiers next (1 Shift, 2 Alt, 4 Ctrl), then the key */
#define HY_O_READ               0         /* Open modes */
#define HY_O_WRITE              1
#define HY_O_RDWR               2
#define HY_O_RW_MASK            3
#define HY_O_TRUNC              0x10      /* Open modes: empty the file first */
#define HY_O_NONBLOCK           0x40      /* Open modes: E_AGAIN rather than wait */
#define HY_FD_MAX               16        /* Fds a task has (0 stdin, 1 stdout, 2 stderr) */
#define HY_PATH_MAX             63        /* A path's length at most */
#define HY_IO_UNIT              512       /* The most a READ or WRITE asks of a server in one request */
#define HY_SR_NAME              0         /* The stat record (SR_SIZE bytes): its name (32, zero-terminated) */
#define HY_SR_QTYPE             32        /* Stat: the qid's type (QT_*) */
#define HY_SR_QVERS             33        /* Stat: the qid's version */
#define HY_SR_QPATH             34        /* Stat: the qid's path (4: unique in its server) */
#define HY_SR_MODE              38        /* Stat: the mode (2: permissions; high byte DM_DIR) */
#define HY_SR_LENGTH            40        /* Stat: the length (4) */
#define HY_SR_MTIME             44        /* Stat: the time it changed (4: seconds since 2000) */
#define HY_SR_DEV               48        /* Stat: the device letter */
#define HY_SR_INST              49        /* Stat: the device's instance (its spec's first character) */
#define HY_SR_SIZE              64
#define HY_QT_DIR               0x80      /* Qid types: a directory */
#define HY_QT_FILE              0x00
#define HY_DM_DIR               0x80      /* A mode's high byte: a directory */
#define HY_DM_APPEND            0x40      /* A mode's high byte: append-only (writes go to the end) */
#define HY_TASK_INBOX           0x0200    /* A server's copy of the request it's serving (RQ_*) */
#define HY_TASK_PATH            0x02C0    /* A server's copy of the name a request names (64 bytes) */
#define HY_RQ_TYPE              0         /* The request block (RQ_SIZE bytes): the request (R_*) */
#define HY_RQ_FID               1         /* Request: the fid (out: the new fid, R_OPEN and R_CREATE) */
#define HY_RQ_MODE              2         /* Request: the open mode */
#define HY_RQ_CLIENT            3         /* Request: the client task */
#define HY_RQ_U                 4         /* Request: the client's U */
#define HY_RQ_FLAGS             5         /* Request: RF_* */
#define HY_RQ_OFFSET            6         /* Request: the offset (4) */
#define HY_RQ_COUNT             10        /* Request: the count asked for (2) */
#define HY_RQ_DONE              12        /* Request: the count done (2, out) */
#define HY_RQ_BUF               14        /* Request: the buffer, in the client's view (2) */
#define HY_RQ_PERM              16        /* Request: R_CREATE's new mode (high byte); out (R_OPEN, R_CREATE): the qid type */
#define HY_RQ_NAMELEN           17        /* Request: the name's length */
#define HY_RQ_GROUP             18        /* Request: the client's note group */
#define HY_RQ_DEV               19        /* Request: the device letter */
#define HY_RQ_SPEC              20        /* Request: the mount's spec (8, zero-padded) */
#define HY_RQ_EVENT             28        /* Request: the server's event count as it took it (after E_AGAIN, the client waits for a change) */
#define HY_RQ_SIZE              32
#define HY_RF_NONBLOCK          0x01      /* Request flags: answer E_AGAIN rather than wait */
#define HY_R_OPEN               1         /* Requests */
#define HY_R_CREATE             2
#define HY_R_READ               3
#define HY_R_WRITE              4
#define HY_R_CLUNK              5
#define HY_R_STAT               6
#define HY_R_WSTAT              7
#define HY_R_REMOVE             8
#define HY_R_FLUSH              9
#define HY_R_DUP                10
#define HY_SND_CHANNELS         24        /* Sound (#a): its channels: 0-7 the YM2151's, 8-23 the Vera X's PSG (its voices 0-15) */
#define HY_SND_PSG              8         /* Sound: the PSG's first channel (its voice 0) */
#define HY_SND_PATCHES          163       /* Sound: its patches (0-127: General MIDI's programs; 128-162: drums and percussion) */
#define HY_SND_DRUMS            128       /* Sound: General MIDI's drums (MIDI channel 10's notes) */
#define HY_SND_R_CH             0x02      /* Sound: a command, at a register the chip doesn't have: the channel the next commands are for (0-23) */
#define HY_SND_R_PATCH          0x03      /* Sound: load patch n into the channel (its speakers stay) */
#define HY_SND_R_NOTE           0x04      /* Sound: key on MIDI note n (60: middle C; 69: A, 440 Hz), with the channel's bend */
#define HY_SND_R_OFF            0x05      /* Sound: key off (the release) */
#define HY_SND_R_VOL            0x06      /* Sound: the channel's volume (0-127, as General MIDI's) */
#define HY_SND_R_PAN            0x07      /* Sound: the channel's speakers (SND_PAN_*) */
#define HY_SND_R_BEND           0x09      /* Sound: the channel's bend (signed: 64ths of a semitone; the note playing too) */
#define HY_SND_R_DRUM           0x0A      /* Sound: a General MIDI drum (its patch and pitch), keyed on */
#define HY_SND_R_FREQ_LO        0x0B      /* Sound: a frequency's low byte (Hz), for SND_R_FREQ (only kept) */
#define HY_SND_R_FREQ           0x0C      /* Sound: key on at a frequency, SND_R_FREQ_LO's byte and this its high one (Hz; 0: key off) */
#define HY_SND_R_GLIDE          0x0D      /* Sound: the channel's pitch to note n without a new attack (legato) */
#define HY_SND_R_WAVE           0x0E      /* Sound: a PSG channel's waveform: SND_WAVE_* << 6 | its pulse width (0-63: 63 a square) */
#define HY_SND_PAN_LEFT         1         /* Sound: SND_R_PAN's speakers */
#define HY_SND_PAN_RIGHT        2
#define HY_SND_PAN_BOTH         3
#define HY_SND_WAVE_PULSE       0         /* Sound: SND_R_WAVE's waveforms (the PSG's) */
#define HY_SND_WAVE_SAW         1
#define HY_SND_WAVE_TRIANGLE    2
#define HY_SND_WAVE_NOISE       3
#define HY_r0                   0x02      /* The call registers r0-r15: arguments and results */
#define HY_r1                   0x04
#define HY_r2                   0x06
#define HY_r3                   0x08
#define HY_r4                   0x0A
#define HY_r5                   0x0C
#define HY_r6                   0x0E
#define HY_r7                   0x10
#define HY_r8                   0x12
#define HY_r9                   0x14
#define HY_r10                  0x16
#define HY_r11                  0x18
#define HY_r12                  0x1A
#define HY_r13                  0x1C
#define HY_r14                  0x1E
#define HY_r15                  0x20

#endif
