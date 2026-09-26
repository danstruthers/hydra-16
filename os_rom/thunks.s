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
