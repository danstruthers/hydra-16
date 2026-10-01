; ****************************************************************************
; The thunk table: the public calls' fixed entry points, $F800 on (docs/programming/rom-layout.md, the API
; index).  One list for both copies: page 0's (kernel/thunks.s: TH_name, a jmp to the call) and page 1's
; (hyforth/page1.s, for code that runs on page 1: HyForth's binary words), which define THUNK and THUNK_P0
; before using THUNK_TABLE.  An entry: its call's name, and its address (checked).
;   THUNK       page 1 has a gate of its own for the call: its entry jumps to it
;   THUNK_P0    page 1 has none: its entry goes on to page 0's (THUNK_TO_P0)
; Add new calls at the end (as THUNK_P0, unless page 1 has the gate anyway).  Never move an entry: programs
; rely on the addresses.

.macro THUNK_TABLE
            THUNK       READ_CHAR,          $F800
            THUNK       WRITE_CHAR,         $F803
            THUNK       WRITE_BYTE,         $F806
            THUNK       WRITE_HEX,          $F809
            THUNK       WRITE_HEX_MASK,     $F80C
            THUNK       WRITE_HSTRING,      $F80F
            THUNK       WRITE_CRLF,         $F812
            THUNK       CLEAR_SCR,          $F815
            THUNK       DISASM,             $F818
            THUNK       DISASM_AY,          $F81B
            THUNK       MEM_COPY,           $F81E
            THUNK       MM_ALLOC,           $F821
            THUNK       MM_FREE,            $F824
            THUNK       MM_READ,            $F827
            THUNK       MM_WRITE,           $F82A
            THUNK       MM_LOCK,            $F82D
            THUNK       MM_UNLOCK,          $F830
            THUNK       MMU_TEST,           $F833
            THUNK       SH_ALLOC,           $F836
            THUNK       SH_ATTACH,          $F839
            THUNK       SH_DETACH,          $F83C
            THUNK       SH_READ,            $F83F
            THUNK       SH_WRITE,           $F842
            THUNK       SH_LOCK,            $F845
            THUNK       SH_UNLOCK,          $F848
            THUNK       MM_TASK_RESET,      $F84B
            THUNK       MM_FIND,            $F84E
            THUNK       MM_SET_FLOOR,       $F851
            THUNK       YIELD,              $F854
            THUNK       NO_PREEMPT,         $F857
            THUNK       PREEMPT,            $F85A
            THUNK       TASK_WAIT,          $F85D
            THUNK       IO_WAKE,            $F860
            THUNK       TASK_RUN,           $F863
            THUNK       TASK_STATUS,        $F866
            THUNK       SCHED_TEST,         $F869
            THUNK       IO_OPEN,            $F86C
            THUNK       IO_CLOSE,           $F86F
            THUNK       IO_READ,            $F872
            THUNK       IO_WRITE,           $F875
            THUNK       IO_GETC,            $F878
            THUNK       IO_PUTC,            $F87B
            THUNK       IO_SEEK,            $F87E
            THUNK       IO_STAT,            $F881
            THUNK       IO_CTL,             $F884
            THUNK       DEV_REGISTER,       $F887
            THUNK       IO_TEST,            $F88A
            THUNK       GET_CHAR,           $F88D
            THUNK       IO_DUP2,            $F890
            THUNK       IO_PIPE,            $F893
            THUNK       IO_DUP,             $F896
            THUNK       TASK_CLONE,         $F899
            THUNK       IO_MOUNT,           $F89C
            THUNK       IO_BIND,            $F89F
            THUNK       IO_UNMOUNT,         $F8A2
            THUNK       IO_NS_LIST,         $F8A5
            THUNK       TASK_SET_BREAK,     $F8A8
            THUNK       TASK_SIGNAL,        $F8AB
            THUNK       CONS_SET_FG,        $F8AE
            THUNK       FP_MAKE,            $F8B1
            THUNK       FP_READ,            $F8B4
            THUNK       FP_WRITE,           $F8B7
            THUNK       FP_COPY,            $F8BA
            THUNK       MM_REF,             $F8BD
            THUNK       MM_FP,              $F8C0
            THUNK       SH_REF,             $F8C3
            THUNK       SH_FP,              $F8C6
            THUNK       IO_CREATE,          $F8C9
            THUNK       IO_REMOVE,          $F8CC
            THUNK       IO_WSTAT,           $F8CF
            THUNK       IO_CHDIR,           $F8D2
            THUNK       IO_GETCWD,          $F8D5
            THUNK_P0    SEM_NEW,            $F8D8
            THUNK_P0    SEM_ACQUIRE,        $F8DB
            THUNK_P0    SEM_TRY,            $F8DE
            THUNK_P0    SEM_RELEASE,        $F8E1
            THUNK_P0    SEM_FREE,           $F8E4
            THUNK_P0    TASK_SLEEP,         $F8E7
            THUNK_P0    TICKS_GET,          $F8EA
            THUNK_P0    CLOCK_GET,          $F8ED
            THUNK_P0    TASK_EXITS,         $F8F0
            THUNK_P0    TASK_JOIN,          $F8F3
            THUNK_P0    SHELL_CMD,          $F8F6
.endmacro
