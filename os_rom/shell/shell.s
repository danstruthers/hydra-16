.debuginfo

; ****************************************************************************
; The shell's routines (BIOS ROM page 7; see page7.s): the boot shell's start, the prompt, and what
; HyForth's shell words (cd, pwd, ...) call.  They run in the shell's task.  Its RAM is in HyForth's image
; (PAGE1::SHBUF, SHBUF2, PROMPTFMT ...: hywords.s), and ZP_TEMP_VEC / ZP_TEMP_VEC2 are their pointers.
; All: C = 0; or C = 1, .A = error.

.segment "SHELL_P7"

SH_PTR          = ZP_TEMP_VEC                               ; A pointer (a format, a name) ...
SH_PTR2         = ZP_TEMP_VEC2                              ;   and another

; ****************************************************************************
; Boot

; The boot shell (task 1, from BOOT_SHELL on page 0): stdio on the console, the cards' files at /sd, the
; volumes found and the lowest selected (SH_VOLUMES), the clock chip (SH_CLOCK), then HyForth, which runs
; boot.hys from there
; (BOOTFLAG), and WOZMON after bye.  (A shell started later starts at SHELL_MAIN, on page 0, and inherits
; its parent's namespace and current directory.)
SH_BOOT:
            jsr         IO_STD_OPEN                         ; fds 0-2 on /dev/cons (inherited by what it starts)
            jsr         ENV_INIT                            ; An empty environment (inherited too), and its
            LOAD_ADDR   ::ENV_SERVE, ZP_TC_VEC              ;   server (env: in each client's task; /env
            lda         #<SH_S_ENV
            ldy         #>SH_S_ENV
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER                        ;   mounted below)
            LOAD_ADDR   ::RAM_SERVE, ZP_TC_VEC              ; The RAM itself, for task 0: /dev/ram (in its
            lda         #<SH_S_RAMDEV                       ;   client's task: task 0's)
            ldy         #>SH_S_RAMDEV
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER
            LOAD_ADDR   ::TIME_SERVE, ZP_TC_VEC             ; The clock: /dev/time (in each client's task too)
            lda         #<SH_S_TIME
            ldy         #>SH_S_TIME
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER
            LOAD_ADDR   ::GPIO_SERVE, ZP_TC_VEC             ; Port A on J27: /dev/gpio (in each client's task)
            lda         #<SH_S_GPIO
            ldy         #>SH_S_GPIO
            ldx         #IO_DEV_CALLER_TASK
            jsr         DEV_REGISTER
            LOAD_ADDR   SH_S_HFS, ZP_IO_BUF                 ; The cards' files at /sd: in the system namespace,
            lda         #<SH_S_SD                           ;   which every task sees (NS_SYSTEM), as all of
            ldy         #>SH_S_SD                           ;   these but /ram (a shell's own)
            ldx         #NS_SYSTEM
            jsr         IO_MOUNT
            LOAD_ADDR   (SH_S_ENVP + 1), ZP_IO_BUF          ; The environment at /env
            lda         #<SH_S_ENVP
            ldy         #>SH_S_ENVP
            ldx         #NS_SYSTEM
            jsr         IO_MOUNT
            LOAD_ADDR   (SH_S_PROC + 1), ZP_IO_BUF          ; The tasks at /proc (the device proc: /dev/proc
            lda         #<SH_S_PROC                         ;   too), as Plan 9 has them
            ldy         #>SH_S_PROC
            ldx         #NS_SYSTEM
            jsr         IO_MOUNT
            ldx         #0                                  ; The disks in memory: /rom, /sram (the shared RAM
                                                            ;   disk), each if it's there (a paged ROM with no
@mount:                                                     ;   ROM disk: no /rom).  Mounts of hfs with a spec,
            phx                                             ;   inherited too (DISKS.md)
            lda         SH_MOUNTS + 2,X
            sta         ZP_IO_CNT
            lda         SH_MOUNTS + 3,X
            sta         ZP_IO_CNT + 1
            lda         SH_MOUNTS,X
            ldy         SH_MOUNTS + 1,X
            ldx         #NS_SYSTEM
            jsr         SH_MOUNT_SPEC
            jsr         SH_IF_THERE
            pla
            clc
            adc         #4
            tax
            cpx         #SH_MOUNTS_END - SH_MOUNTS
            bne         @mount
            jsr         SH_RAM_DIRS                         ; The shared program caches' directories
            jsr         SH_OWN_AREA                         ; This shell's area, at /ram
            jsr         SH_VOLUMES                          ; (.A = the boot script: BOOTFLAG's, after
            pha                                             ;   COPYTORAM sets HyForth's variables)
            jsr         SH_CLOCK
            jsr         COPYTORAM
            pla                                             ; (HyForth: run boot.hys before the first prompt:
            sta         PAGE1::BOOTFLAG                     ;   a card's, or with none the ROM's)
            jsr         forth_main
            jmp         MON_START

SH_S_SD:    .byte   "/sd", 0
SH_S_PROC:  .byte   "/proc", 0                          ; (And the device's name: "proc")
SH_S_ENVP:  .byte   "/env", 0                           ; (... "env")
SH_S_ENV    = SH_S_ENVP + 1
SH_S_TIME:  .byte   "time", 0
SH_S_RAMDEV: .byte  "ram", 0
SH_S_GPIO:  .byte   "gpio", 0
SH_S_HFS:   .byte   "hfs", 0
SH_MOUNTS:  .word   SH_S_ROM, SH_S_SPEC_ROM             ; The boot shell's mounts of hfs: a name, its spec
            .word   SH_S_SRAM, SH_S_SPEC_SRAM           ;   (the ROM disk; the shared RAM disk)
SH_MOUNTS_END:
SH_S_ROM:   .byte   "/rom", 0
SH_S_SPEC_ROM: .byte DISK_NAME_ROM, 0
SH_S_SRAM:  .byte   "/sram", 0
SH_S_SPEC_SRAM: .byte DISK_NAME_SRAM, 0
SH_S_RAM:   .byte   "/ram", 0
SH_S_VOLS:  .byte   "hydrafs", 0

; This shell's own area on the RAM disk, r/N (N: its task), mounted at /ram (mount hfs /ram r/N), and made, with
; its bin and lib (its caches: /ram/bin and /ram/lib, the first members of /bin and /lib): the boot shell's
; (SH_BOOT), and each shell's started later (SHELL_MAIN), so the programs, scripts and pipelines a shell runs use
; its area, as they inherit its namespace (docs/plans/DISKS.md).  (No RAM disk: no /ram.)  Modifies: .A, .X, .Y
SH_OWN_AREA:
            lda         #DISK_NAME_RAM                      ; PAGE1::SHBUF2 = the spec: r/N
            sta         PAGE1::SHBUF2
            lda         #'/'
            sta         PAGE1::SHBUF2 + 1
            lda         T_REGISTER
            and         #$0F
            cmp         #10
            bcc         :+
            adc         #'a' - '0' - 10 - 1                 ; (C = 1)
:
            adc         #'0'
            sta         PAGE1::SHBUF2 + 2
            stz         PAGE1::SHBUF2 + 3
            LOAD_ADDR   PAGE1::SHBUF2, ZP_IO_CNT
            lda         #<SH_S_RAM                          ; (In this shell's own namespace: .X = 0)
            ldy         #>SH_S_RAM
            ldx         #0
            jsr         SH_MOUNT_SPEC
            jsr         SH_MKDIR                            ; The area itself (there already: as it is) ...
            lda         #<SH_S_RAM                          ; ... its bin and lib
            ldy         #>SH_S_RAM
            jsr         SH_BUF_DIRS
            lda         #<SH_S_RAM                          ; There?  (Else no /ram)
            ldy         #>SH_S_RAM
            ldx         #0
            bra         SH_IF_THERE

; Mount hfs at .A.Y with the spec at ZP_IO_CNT (SH_MOUNTS, SH_OWN_AREA), in the namespace .X says: NS_SYSTEM, or
; 0, this task's.  Preserves .A, .X, .Y
SH_MOUNT_SPEC:
            phx
            pha
            LOAD_ADDR   SH_S_HFS, ZP_IO_BUF                 ; (It changes .A)
            txa
            ora         #NS_SPEC
            tax
            pla
            pha
            jsr         IO_MOUNT                            ; (Preserves .X, .Y)
            pla
            plx
            rts

; The mount at .A.Y kept if its disk is there (the path opens); else unmounted (from the namespace .X says, as
; SH_MOUNT_SPEC's).  Modifies: .A, .X, .Y
SH_IF_THERE:
            pha
            phy
            phx
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @gone
            jsr         IO_CLOSE
            plx
            ply
            pla
            rts

@gone:
            plx
            ply
            pla
            jmp         IO_UNMOUNT

; The shared program caches' directories, on the shared RAM disk: /sram/bin and /sram/lib.  (Errors, or no
; RAM disk: left as they are.)
SH_RAM_DIRS:
            lda         #<SH_S_SRAM
            ldy         #>SH_S_SRAM

; SHBUF = the directory at .A.Y (on this page), and its bin and lib made (SH_RAM_SUBDIRS).  Modifies: .A, .X, .Y
SH_BUF_DIRS:
            jsr         SH_KEEP                             ; (SH_PTR)
            ldy         #0
:
            lda         (SH_PTR),Y
            sta         PAGE1::SHBUF,Y
            beq         :+
            iny
            bra         :-
:
            tya
            tax

; SHBUF up to .X: a directory to make /bin and /lib in (SH_S_DIRS' names)
SH_RAM_SUBDIRS:
            stx         PAGE1::SHN
            ldy         #0
            jsr         @one
            ldy         #SH_S_DIRS_LEN

@one:
            ldx         PAGE1::SHN
            lda         #SH_S_DIRS_LEN - 1                  ; ("/bin": not its last '/')
            sta         PAGE1::SHSEL
:
            lda         SH_S_DIRS,Y
            sta         PAGE1::SHBUF,X
            inx
            iny
            dec         PAGE1::SHSEL
            bne         :-
            stz         PAGE1::SHBUF,X

SH_MKDIR_BUF:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            jmp         SH_MKDIR

; Find the HydraFS volumes: /sd/0 ... /sd/7 opened (so each card is started, and its superblock read),
; the ones that have one listed ("hydrafs 0 2"), and the lowest made the current directory (none: this
; task's own area on the RAM disk, /ram).  OUT: .A = the boot script (BOOTFLAG): 1, boot.hys there (a card's);
; 2, with no card, /rom/boot.hys
SH_VOLUMES:
            stz         PAGE1::SHN
            lda         #$FF
            sta         PAGE1::SHSEL

@card:
            lda         PAGE1::SHN
            jsr         SH_CARD_PATH                        ; SHBUF = "/sd/N"
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @next                               ; (No card, or no HydraFS on it)
            jsr         IO_CLOSE
            lda         PAGE1::SHSEL
            bpl         @listed
            lda         PAGE1::SHN                          ; The first: selected
            sta         PAGE1::SHSEL
            lda         #<SH_S_VOLS
            ldy         #>SH_S_VOLS
            jsr         SH_PUTS

@listed:
            lda         #' '
            jsr         WRITE_CHAR
            lda         PAGE1::SHN
            ora         #'0'
            jsr         WRITE_CHAR

@next:
            inc         PAGE1::SHN
            lda         PAGE1::SHN
            cmp         #SD_MAX_CARDS
            bne         @card
            lda         PAGE1::SHSEL
            bmi         @no_card                            ; (None)
            jsr         SH_CRLF
            lda         PAGE1::SHSEL
            jsr         SH_CARD_PATH
            jsr         SH_CARD_BINS                        ; Its bin and lib: /bin and /lib

@chdir:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF

@chdir_ay:
            jsr         IO_CHDIR
            lda         #1                                  ; (Its boot script: a card's, or the ROM's)
            ldx         PAGE1::SHSEL
            bpl         :+
            inc
:
            clc
            rts

@no_card:                                                   ; No card: this task's own area on the RAM disk,
            lda         #<SH_S_RAM                          ;   /ram, so files can be saved (until a reset).
            ldy         #>SH_S_RAM                          ;   (No RAM disk: "/", as it was)
            bra         @chdir_ay

; The clock chip: looked for (RTC_BOOT, page 9: the clock set from it, if there's one), and its line printed:
; "clock 2026-09-30 14:05:00", "clock stopped: set the time" or "no clock"
SH_CLOCK:
            LOAD_ADDR   PAGE1::SHBUF2, ZP_IO_REQ            ; (RTC_BOOT's text: at (ZP_IO_REQ), ZP_PROC_IDX long)
            jsr         RTC_BOOT
            ldx         #0
:
            lda         PAGE1::SHBUF2,X
            jsr         WRITE_CHAR
            inx
            cpx         ZP_PROC_IDX
            bne         :-
            rts

; The boot card's bin and lib (SHBUF = its root, "/sd/N") bound at /bin and /lib: they start those unions, and
; /rom/lib/namespace puts the program caches before them and the ROM's after (docs/plans/NAMESPACES.md).  SHBUF
; stays the root.  Modifies: .A, .X, .Y
SH_BINLIB_LEN   = 5
SH_CARD_PATH_LEN = 5                                    ; ("/sd/N")
SH_CARD_BINS:
            ldx         #0

@dir:
            ldy         #0
:
            lda         SH_S_BINLIB,X                       ; SHBUF = /sd/N/bin (/lib)
            sta         PAGE1::SHBUF + SH_CARD_PATH_LEN,Y
            inx
            iny
            cmp         #0
            bne         :-
            phx
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF
            txa                                             ; .A.Y = its /bin (/lib)
            sec
            sbc         #SH_BINLIB_LEN
            clc
            adc         #<SH_S_BINLIB
            ldy         #>SH_S_BINLIB
            bcc         :+
            iny
:
            ldx         #NS_SYSTEM
            jsr         IO_BIND
            plx
            cpx         #SH_S_BINLIB_END - SH_S_BINLIB
            bne         @dir
            stz         PAGE1::SHBUF + SH_CARD_PATH_LEN
            rts

SH_S_BINLIB:    .byte   "/bin", 0, "/lib", 0
SH_S_BINLIB_END:

; SHBUF = card .A's root, "/sd/N".  Modifies: .A
SH_CARD_PATH:
            ora         #'0'
            sta         PAGE1::SHBUF + 4
            lda         #'/'
            sta         PAGE1::SHBUF
            sta         PAGE1::SHBUF + 3
            lda         #'s'
            sta         PAGE1::SHBUF + 1
            lda         #'d'
            sta         PAGE1::SHBUF + 2
            stz         PAGE1::SHBUF + 5
            rts

; ****************************************************************************
; The prompt

; Print the prompt, on a new line (HyForth's words don't end their output with one): the format at .A.Y
; (zero-terminated), with its fields filled in:
;   %v  the volume: "N:" when the current directory is on card N, nothing off the cards
;   %d  the directory: its path on the card ("/games", "/" at its root), or the whole path off the cards
;   %p  the whole path ("/sd/0/games")
;   %l  the card's HydraFS label (SH_LABEL), nothing off the cards
;   %t  the task (0-F)
;   %%  a %
; PROMPTLAST = the last character printed (HyForth puts it back when the console's echo erases it).
SH_PROMPT:
            sta         SH_PTR
            sty         SH_PTR + 1
            jsr         SH_CRLF
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF             ; SHBUF = the current directory
            jsr         IO_GETCWD
            jsr         SH_ON_CARD                          ; SHN = the card ($FF: none), .X = the rest
            stx         PAGE1::SHSEL                        ;   (SHSEL: where the path on the card starts)
            ldy         #0

@char:
            lda         (SH_PTR),Y
            beq         @done
            iny
            cmp         #'%'
            bne         @print
            lda         (SH_PTR),Y                          ; A field
            beq         @done
            iny
            cmp         #'v'
            beq         @volume
            cmp         #'d'
            beq         @directory
            cmp         #'p'
            beq         @path
            cmp         #'l'
            beq         @label
            cmp         #'t'
            bne         @print                              ; (%% and anything else: itself)
            lda         T_REGISTER
            and         #$0F
            ora         #'0'
            cmp         #'9' + 1
            bcc         @print
            adc         #'A' - '9' - 2                      ; (C = 1)

@print:
            sta         PAGE1::PROMPTLAST
            jsr         WRITE_CHAR
            bra         @char

@volume:
            lda         PAGE1::SHN
            bmi         @char
            ora         #'0'
            jsr         WRITE_CHAR
            lda         #':'
            bra         @print

@directory:
            ldx         PAGE1::SHSEL                        ; On a card: the path on it ("/" at its root)
            lda         PAGE1::SHN
            bmi         @from_x                             ; (Off the cards: the whole path, from .X = 0)
            lda         PAGE1::SHBUF,X
            bne         @from_x
            lda         #'/'
            bra         @print

@path:
            ldx         #0

@from_x:                                                    ; SHBUF from .X on
            lda         PAGE1::SHBUF,X
            beq         @char
            sta         PAGE1::PROMPTLAST
            jsr         WRITE_CHAR
            inx
            bra         @from_x

@label:
            lda         PAGE1::SHN                          ; On a card: its label
            bmi         @char
            jsr         SH_LABEL
            ldx         #0
:
            lda         PAGE1::SHLABEL,X
            beq         @char
            sta         PAGE1::PROMPTLAST
            jsr         WRITE_CHAR
            inx
            bra         :-

@done:
            clc
            rts

; SHLABEL = card SHN's HydraFS label: after "hydrafs label=" in its ctl file's text (the text's first
; '='), or nothing (no HydraFS).  Read only when the card isn't LBLCARD's, the card it was read for
; (mkfs and relabel make that $FF).  Uses SHBUF2, SHOWBUF, SHFD.  Preserves .Y and SH_PTR
SH_LABEL:
            lda         PAGE1::SHN
            cmp         PAGE1::LBLCARD
            beq         @done
            sta         PAGE1::LBLCARD
            stz         PAGE1::SHLABEL                      ; (None, unless it's found)
            phy
            lda         SH_PTR
            pha
            lda         SH_PTR + 1
            pha
            ldx         #SH_S_CTLPATH_END - SH_S_CTLPATH - 1 ; SHBUF2 = "/dev/sd/N/ctl"
:
            lda         SH_S_CTLPATH,X
            sta         PAGE1::SHBUF2,X
            dex
            bpl         :-
            lda         PAGE1::SHN
            ora         #'0'
            sta         PAGE1::SHBUF2 + 8
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @out
            sta         PAGE1::SHFD
            jsr         SH_READ                             ; SHOWBUF = its text ...
            ldx         ZP_IO_CNT                           ;   ends with a 0 (the last byte, if it's full)
            lda         ZP_IO_CNT + 1
            beq         :+
            ldx         #PAGE1::SHOWBUF_SIZE - 1
:
            stz         PAGE1::SHOWBUF,X
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            ldx         #0

@find:                                                      ; The first '='
            lda         PAGE1::SHOWBUF,X
            beq         @out
            inx
            cmp         #'='
            bne         @find
            ldy         #0

@copy:                                                      ; The label: to the line's end
            lda         PAGE1::SHOWBUF,X
            cmp         #ASCII_CR
            beq         @end
            cmp         #ASCII_LF
            beq         @end
            sta         PAGE1::SHLABEL,Y
            beq         @out
            inx
            iny
            cpy         #HFS_NAME_MAX
            bne         @copy

@end:
            lda         #0
            sta         PAGE1::SHLABEL,Y

@out:
            pla
            sta         SH_PTR + 1
            pla
            sta         SH_PTR
            ply

@done:
            rts

; Is SHBUF's path on a card: "/sd/N" or "/sd/N/..."?  OUT: SHN = the card, .X = where the path on it
; starts (5); or SHN = $FF, .X = 0.  Modifies: .A
SH_ON_CARD:
            ldx         #3                                  ; "/sd/"
:
            lda         PAGE1::SHBUF,X
            cmp         SH_S_SDPRE,X
            bne         @no
            dex
            bpl         :-
            lda         PAGE1::SHBUF + 4                    ; A card's number
            sec
            sbc         #'0'
            cmp         #SD_MAX_CARDS
            bcs         @no
            sta         PAGE1::SHN
            lda         PAGE1::SHBUF + 5                    ; ... and the end, or a '/'
            beq         :+
            cmp         #'/'
            bne         @no
:
            ldx         #5
            rts

@no:
            lda         #$FF
            sta         PAGE1::SHN
            ldx         #0
            rts

SH_S_SDPRE: .byte   "/sd/"
SH_S_HOME:  .byte   "/env/HOME", 0

; ****************************************************************************
; Directories

; Change directory: to the path at .A.Y (relative, or not); or, with .Y = 0 (no path), to $HOME if it's set,
; or else to the current card's root (or "/" off the cards).
SH_CD:
            cpy         #0
            bne         @path
            lda         #<SH_S_HOME                         ; $HOME, if it's set
            ldy         #>SH_S_HOME
            jsr         SH_ENV_READ
            bcs         :+
            lda         #<PAGE1::SHOWBUF
            ldy         #>PAGE1::SHOWBUF
            bra         @path
:
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF             ; Or the card's root: the current directory,
            jsr         IO_GETCWD                           ;   cut after "/sd/N"
            jsr         SH_ON_CARD
            stz         PAGE1::SHBUF,X                      ; (Off the cards: "", which IO_CHDIR takes as
            txa                                             ;   the current directory: so "/" instead)
            bne         :+
            lda         #'/'
            sta         PAGE1::SHBUF
            stz         PAGE1::SHBUF + 1
:
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF

@path:
            jmp         IO_CHDIR

; Print the current directory
SH_PWD:
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF
            jsr         IO_GETCWD
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            jsr         SH_PUTS
            clc
            rts

; Print CR LF.  OUT: C = 0.  Modifies: .A
SH_CRLF:
            lda         #ASCII_CR
            jsr         WRITE_CHAR
            lda         #ASCII_LF
            jsr         WRITE_CHAR
            clc
            rts

; Print the zero-terminated text at .A.Y (in RAM, or on this page).  Modifies: .A, .Y
SH_PUTS:
            sta         SH_PTR2
            sty         SH_PTR2 + 1
            ldy         #0
:
            lda         (SH_PTR2),Y
            beq         :+
            jsr         WRITE_CHAR
            iny
            bra         :-
:
            rts
