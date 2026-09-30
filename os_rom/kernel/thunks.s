.debuginfo

.segment "BIOS_THUNKS"
TH_READ_CHAR:
                jmp             READ_CHAR           ; $F800
TH_WRITE_CHAR:
                jmp             WRITE_CHAR          ; $F803
TH_WRITE_BYTE:
                jmp             WRITE_BYTE          ; $F806
TH_WRITE_HEX:
                jmp             WRITE_HEX           ; $F809
TH_WRITE_HEX_MASK:
                jmp             WRITE_HEX_MASK      ; $F80C
TH_WRITE_HSTRING:
                jmp             WRITE_HSTRING       ; $F80F
TH_WRITE_CRLF:
                jmp             WRITE_CRLF          ; $F812
TH_CLEAR_SCR:
                jmp             CLEAR_SCR           ; $F815
TH_DISASM:
                jmp             DISASM              ; $F818
TH_DISASM_AY:
                jmp             DISASM_AY           ; $F81B
TH_MEM_COPY:
                jmp             MEM_COPY            ; $F81E
TH_MM_ALLOC:
                jmp             MM_ALLOC            ; $F821
TH_MM_FREE:
                jmp             MM_FREE             ; $F824
TH_MM_READ:
                jmp             MM_READ             ; $F827
TH_MM_WRITE:
                jmp             MM_WRITE            ; $F82A
TH_MM_LOCK:
                jmp             MM_LOCK             ; $F82D
TH_MM_UNLOCK:
                jmp             MM_UNLOCK           ; $F830
TH_MMU_TEST:
                jmp             MMU_TEST            ; $F833
TH_SH_ALLOC:
                jmp             SH_ALLOC            ; $F836
TH_SH_ATTACH:
                jmp             SH_ATTACH           ; $F839
TH_SH_DETACH:
                jmp             SH_DETACH           ; $F83C
TH_SH_READ:
                jmp             SH_READ             ; $F83F
TH_SH_WRITE:
                jmp             SH_WRITE            ; $F842
TH_SH_LOCK:
                jmp             SH_LOCK             ; $F845
TH_SH_UNLOCK:
                jmp             SH_UNLOCK           ; $F848
TH_MM_TASK_RESET:
                jmp             MM_TASK_RESET       ; $F84B
TH_MM_FIND:
                jmp             MM_FIND             ; $F84E
TH_MM_SET_FLOOR:
                jmp             MM_SET_FLOOR        ; $F851
TH_YIELD:
                jmp             YIELD               ; $F854
TH_NO_PREEMPT:
                jmp             NO_PREEMPT          ; $F857
TH_PREEMPT:
                jmp             PREEMPT             ; $F85A
TH_TASK_WAIT:
                jmp             TASK_WAIT           ; $F85D
TH_IO_WAKE:
                jmp             IO_WAKE             ; $F860
TH_TASK_RUN:
                jmp             TASK_RUN            ; $F863
TH_TASK_STATUS:
                jmp             TASK_STATUS         ; $F866
TH_SCHED_TEST:
                jmp             SCHED_TEST          ; $F869
TH_IO_OPEN:
                jmp             IO_OPEN             ; $F86C
TH_IO_CLOSE:
                jmp             IO_CLOSE            ; $F86F
TH_IO_READ:
                jmp             IO_READ             ; $F872
TH_IO_WRITE:
                jmp             IO_WRITE            ; $F875
TH_IO_GETC:
                jmp             IO_GETC             ; $F878
TH_IO_PUTC:
                jmp             IO_PUTC             ; $F87B
TH_IO_SEEK:
                jmp             IO_SEEK             ; $F87E
TH_IO_STAT:
                jmp             IO_STAT             ; $F881
TH_IO_CTL:
                jmp             IO_CTL              ; $F884
TH_DEV_REGISTER:
                jmp             DEV_REGISTER        ; $F887
TH_IO_TEST:
                jmp             IO_TEST             ; $F88A
TH_GET_CHAR:
                jmp             GET_CHAR            ; $F88D
TH_IO_DUP2:
                jmp             IO_DUP2             ; $F890
TH_IO_PIPE:
                jmp             IO_PIPE             ; $F893
TH_IO_DUP:
                jmp             IO_DUP              ; $F896
TH_TASK_CLONE:
                jmp             TASK_CLONE          ; $F899
TH_IO_MOUNT:
                jmp             IO_MOUNT            ; $F89C
TH_IO_BIND:
                jmp             IO_BIND             ; $F89F
TH_IO_UNMOUNT:
                jmp             IO_UNMOUNT          ; $F8A2
TH_IO_NS_LIST:
                jmp             IO_NS_LIST          ; $F8A5
TH_TASK_SET_BREAK:
                jmp             TASK_SET_BREAK      ; $F8A8
TH_TASK_SIGNAL:
                jmp             TASK_SIGNAL         ; $F8AB
TH_CONS_SET_FG:
                jmp             CONS_SET_FG         ; $F8AE
TH_FP_MAKE:
                jmp             FP_MAKE             ; $F8B1
TH_FP_READ:
                jmp             FP_READ             ; $F8B4
TH_FP_WRITE:
                jmp             FP_WRITE            ; $F8B7
TH_FP_COPY:
                jmp             FP_COPY             ; $F8BA
TH_MM_REF:
                jmp             MM_REF              ; $F8BD
TH_MM_FP:
                jmp             MM_FP               ; $F8C0
TH_SH_REF:
                jmp             SH_REF              ; $F8C3
TH_SH_FP:
                jmp             SH_FP               ; $F8C6
TH_IO_CREATE:
                jmp             IO_CREATE           ; $F8C9
TH_IO_REMOVE:
                jmp             IO_REMOVE           ; $F8CC
TH_IO_WSTAT:
                jmp             IO_WSTAT            ; $F8CF
TH_IO_CHDIR:
                jmp             IO_CHDIR            ; $F8D2
TH_IO_GETCWD:
                jmp             IO_GETCWD           ; $F8D5
TH_SEM_NEW:
                jmp             SEM_NEW             ; $F8D8
TH_SEM_ACQUIRE:
                jmp             SEM_ACQUIRE         ; $F8DB
TH_SEM_TRY:
                jmp             SEM_TRY             ; $F8DE
TH_SEM_RELEASE:
                jmp             SEM_RELEASE         ; $F8E1
TH_SEM_FREE:
                jmp             SEM_FREE            ; $F8E4
TH_TASK_SLEEP:
                jmp             TASK_SLEEP          ; $F8E7
TH_TICKS_GET:
                jmp             TICKS_GET           ; $F8EA
TH_CLOCK_GET:
                jmp             CLOCK_GET           ; $F8ED
TH_TASK_EXITS:
                jmp             TASK_EXITS          ; $F8F0
TH_TASK_JOIN:
                jmp             TASK_JOIN           ; $F8F3
TH_SHELL_CMD:
                jmp             SHELL_CMD           ; $F8F6: a task's entry point (TASK_RUN, page 0): the command
                                                    ;   shell (HyForth running its stdin: Plan 9's rc -c; system())

; Gates for the calls above on other pages, and the kernel's to them (GATES_P0, before the thunks, is full)
FAR_GATE_INLINE TASK_EXITS,     PAGE5::TASK_EXITS,      5
FAR_GATE_INLINE TASK_JOIN,      PAGE5::TASK_JOIN,       5
FAR_GATE_INLINE EXIT_NOTE,      PAGE5::EXIT_NOTE,       5   ; (TASK_EXIT: 0)
FAR_GATE_INLINE EXIT_SIGNALLED, PAGE5::EXIT_SIGNALLED,  5   ; (BREAK_ENTRY: interrupt, killed)
FAR_GATE_INLINE SHELL_CMD,      ::SH_CMDSHELL_P7,       7
FAR_GATE_INLINE YM_BEEP,        PAGEB::YM_BEEP,         $B  ; (The console bell, page B: beep.s, for the serial driver)

