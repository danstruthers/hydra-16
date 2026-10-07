; ****************************************************************************
; mem.s - rc's memory: the arena and the heap, both in RAM BREAK gives it as it starts (so the kernel doesn't clear
; them: rc starts fast), and lists of words.
;
; The arena holds what a command needs while it runs: its tree, its text's copy, what expanding its words makes.
; It's a stack: arena_mark, then arena_release back to the mark (a command's end; a function's; a loop's turn).
; The heap holds what outlives a command: variables, their values, functions.  Its blocks each start with their
; size (2 bytes, with the block's own; bit 15: in use); a free block is merged with the free ones after it as the
; heap is looked through.
;
; A list of words is each word's length (0-254) and its bytes, then LIST_END.

.include "rc.inc"

.zeropage
mp:         .res        2                                   ; (The heap's walk)
mq:         .res        2
ms:         .res        2                                   ; (A size)

.bss
arena_base: .res        2                                   ; The arena's start ...
arena_top:  .res        2                                   ;   and end
heap_base:  .res        2                                   ; The heap's start ...
heap_top:   .res        2                                   ;   and end
la_start:   .res        2                                   ; (list_alloc_word's list)

.code

; The arena and the heap: ARENA_SIZE and HEAP_SIZE bytes from the break up (BREAK moves it).  OUT: C = 0; or C = 1,
; .A = BREAK's error
mem_init:
            stz         r0
            stz         r0 + 1
            jsr         BREAK                               ; (Where it is)
            MOVR        arena_base, r0
            MOVR        ap, r0
            clc
            lda         r0
            adc         #<ARENA_SIZE
            sta         arena_top
            sta         heap_base
            lda         r0 + 1
            adc         #>ARENA_SIZE
            sta         arena_top + 1
            sta         heap_base + 1
            clc
            lda         heap_base
            adc         #<HEAP_SIZE
            sta         heap_top
            sta         r0
            lda         heap_base + 1
            adc         #>HEAP_SIZE
            sta         heap_top + 1
            sta         r0 + 1
            jsr         BREAK
            bcs         @done
            MOVR        mp, heap_base                       ; One free block, the whole heap
            ldy         #0
            lda         #<HEAP_SIZE
            sta         (mp),Y
            iny
            lda         #>HEAP_SIZE
            sta         (mp),Y
            clc
@done:
            rts

.assert     HEAP_SIZE < $8000, error, "A heap block's size has bit 15 free"

; .A/.X bytes from the arena.  OUT: .A/.X = them (an error if there's no room: rc_abort)
arena_alloc:
            pha
            clc
            adc         ap
            sta         ms
            txa
            adc         ap + 1
            sta         ms + 1
            pla
            bcs         @full
            lda         ms                                  ; (Past its end?)
            cmp         arena_top
            lda         ms + 1
            sbc         arena_top + 1
            bcs         @full
            lda         ap
            ldx         ap + 1
            pha
            MOVR        ap, ms
            pla
            rts

@full:
            LDR         r0, s_full
            jmp         rc_error

; .A/.X = the arena's top now (a mark)
arena_mark:
            lda         ap
            ldx         ap + 1
            rts

; The arena back to mark .A/.X
arena_release:
            sta         ap
            stx         ap + 1
            rts

; .A/.X bytes from the heap.  OUT: .A/.X = them (an error if there's no room: rc_abort)
heap_alloc:
            clc                                             ; ms: the block's size, with its own (2), and even
            adc         #3
            and         #$FE
            sta         ms
            txa
            adc         #0
            sta         ms + 1
            MOVR        mp, heap_base
@block:
            lda         mp                                  ; (The end: no room)
            cmp         heap_top
            lda         mp + 1
            sbc         heap_top + 1
            bcc         :+
            jmp         @full
:
            ldy         #1
            lda         (mp),Y
            bpl         @merge
            jmp         @next                               ; (In use)

@merge:
            clc                                             ; mq: the block after it; free?  Then it's this one's
            lda         (mp)
            adc         mp
            sta         mq
            lda         (mp),Y
            adc         mp + 1
            sta         mq + 1
            lda         mq
            cmp         heap_top
            lda         mq + 1
            sbc         heap_top + 1
            bcs         @fits
            lda         (mq),Y
            bmi         @fits
            clc
            lda         (mp)
            adc         (mq)
            sta         (mp)
            lda         (mp),Y
            adc         (mq),Y
            sta         (mp),Y
            bra         @merge

@fits:
            lda         (mp)                                ; Big enough?
            cmp         ms
            lda         (mp),Y
            sbc         ms + 1
            bcs         :+
            jmp         @next
:
            lda         (mp)                                ; mq: what's left of it (4 bytes at least: a block of
            sec                                             ;   its own)
            sbc         ms
            sta         mq
            lda         (mp),Y
            sbc         ms + 1
            sta         mq + 1
            bne         @split
            lda         mq
            cmp         #4
            bcc         @whole
@split:
            clc                                             ; The rest, a free block after it
            lda         mp
            adc         ms
            sta         t0
            lda         mp + 1
            adc         ms + 1
            pha
            lda         t0
            sta         ms
            pla
            sta         ms + 1                              ; (ms: the rest's place)
            lda         mq
            sta         (ms)
            lda         mq + 1
            sta         (ms),Y
            sec                                             ; This one: what was asked
            lda         ms
            sbc         mp
            sta         (mp)
            lda         ms + 1
            sbc         mp + 1
            sta         (mp),Y
@whole:
            lda         (mp),Y                              ; In use
            ora         #$80
            sta         (mp),Y
            clc
            lda         mp
            adc         #2
            pha
            lda         mp + 1
            adc         #0
            tax
            pla
            rts

@next:
            clc
            lda         (mp)
            adc         mp
            pha
            lda         (mp),Y
            and         #$7F
            adc         mp + 1
            sta         mp + 1
            pla
            sta         mp
            jmp         @block

@full:
            LDR         r0, s_full
            jmp         rc_error

; The heap's block .A/.X (from heap_alloc) free again
heap_free:
            sec
            sbc         #2
            sta         mp
            txa
            sbc         #0
            sta         mp + 1
            ldy         #1
            lda         (mp),Y
            and         #$7F
            sta         (mp),Y
            rts

; .A = the length of the string at p0 (255 at most)
str_len:
            ldy         #0
:
            lda         (p0),Y
            beq         :+
            iny
            bne         :-
            dey
:
            tya
            rts

; .A/.X = the words in the list at .A/.X (255 at most)
list_count:
            sta         mp
            stx         mp + 1
            ldx         #0
@word:
            lda         (mp)
            cmp         #LIST_END
            beq         @done
            jsr         list_mp_next
            inx
            bne         @word
            dex
@done:
            txa
            ldx         #0
            rts

; mp past its word
list_mp_next:
            sec
            lda         (mp)
            adc         mp
            sta         mp
            bcc         :+
            inc         mp + 1
:
            rts

; .A/.X = the word after the word at .A/.X
list_next:
            sta         mp
            stx         mp + 1
            jsr         list_mp_next
            lda         mp
            ldx         mp + 1
            rts

; The list at .A/.X copied into the heap.  OUT: .A/.X = the copy
list_copy_heap:
            sta         mq
            stx         mq + 1
            sta         mp                                  ; Its size: to its end, and the end
            stx         mp + 1
@word:
            lda         (mp)
            cmp         #LIST_END
            beq         :+
            jsr         list_mp_next
            bra         @word
:
            sec
            lda         mp
            sbc         mq
            sta         ms
            lda         mp + 1
            sbc         mq + 1
            sta         ms + 1
            inc         ms
            bne         :+
            inc         ms + 1
:
            PUSHW       mq                                  ; (The source and the size: kept across heap_alloc,
            lda         ms + 1                              ;   which uses them)
            pha
            lda         ms
            pha
            ldx         ms + 1
            jsr         heap_alloc
            sta         mp
            stx         mp + 1
            pla
            sta         ms
            pla
            sta         ms + 1
            PULLW       mq
            lda         mp                                  ; (The copy's place, kept)
            pha
            lda         mp + 1
            pha
            ldy         #0                                  ; ms bytes from mq to mp
@copy:
            lda         ms
            ora         ms + 1
            beq         @copied
            lda         (mq),Y
            sta         (mp),Y
            iny
            bne         :+
            inc         mq + 1
            inc         mp + 1
:
            lda         ms
            bne         :+
            dec         ms + 1
:
            dec         ms
            bra         @copy

@copied:
            pla
            tax
            pla
            rts

; A list started at the arena's top.  OUT: .A/.X = it (list_append_word adds words, list_end ends it: nothing
; else from the arena meanwhile)
list_start:
            lda         ap
            ldx         ap + 1
            rts

; A word appended to the list at the arena's top: .A bytes at p1
list_append_word:
            pha
            ldx         #0
            inc         a
            bne         :+
            inx
:
            jsr         arena_alloc
            sta         mp
            stx         mp + 1
            pla
            sta         (mp)
            tax
            beq         @done
            ldy         #0
:
            lda         (p1),Y
            iny
            sta         (mp),Y
            dex
            bne         :-
@done:
            rts

; The list at the arena's top ended
list_end:
            lda         #1
            ldx         #0
            jsr         arena_alloc
            sta         mp
            stx         mp + 1
            lda         #LIST_END
            sta         (mp)
            rts

; An arena word: .A bytes at p1, as a list of one.  OUT: .A/.X = the list
list_alloc_word:
            pha
            jsr         list_start
            sta         la_start                            ; (Not ms: arena_alloc's)
            stx         la_start + 1
            pla
            jsr         list_append_word
            jsr         list_end
            lda         la_start
            ldx         la_start + 1
            rts

.rodata
s_full:     .byte       "out of memory", 0
