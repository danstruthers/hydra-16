\ proc.fs - the tasks, by their files (#p, /proc/N, N in decimal): lib proc.  hylang's proc library's words, its
\ order (the task first).  A task's files: args, cwd, env (its variables, name=value lines), ns (the binds and mounts
\ that made its namespace), regs (its registers as it was switched out), mem (its 64K as it sees it) and ram (its
\ banks: bank b's byte o at b * 8192 + o); mem, ram and regs aren't the kernel task's or a driver's (E_PERM).  The
\ texts are in a buffer of the library's (512 bytes), till the next.  A failure THROWs.  Only the words forth starts
\ with.

create proc-buf 32 allot   variable proc-n        \ A name being made
create proc-out 512 allot                         \ A text read
variable proc-fid

: proc+ ( c-addr u -- ) tuck proc-buf proc-n @ + swap move proc-n +! ;
: proc-file ( task c-addr u -- c-addr2 u2 )       \ /proc/task/name
  rot 0 proc-n ! s" /proc/" proc+ base @ >r decimal 0 <# #s #> r> base ! proc+ s" /" proc+ proc+ proc-buf proc-n @ ;
: proc-read ( task c-addr u -- c-addr2 u2 )
  proc-file r/o open-file throw >r proc-out 512 r@ read-file r> close-file throw throw proc-out swap ;
: proc-open ( task c-addr u ud -- )               \ The file, at offset ud
  2>r proc-file r/o bin open-file throw proc-fid ! 2r> proc-fid @ reposition-file throw ;
: proc-get ( c-addr u -- ) proc-fid @ read-file throw drop proc-fid @ close-file throw ;

: task-args ( task -- c-addr u ) s" args" proc-read ;
: task-cwd ( task -- c-addr u ) s" cwd" proc-read ;
: task-env ( task -- c-addr u ) s" env" proc-read ;
: task-ns ( task -- c-addr u ) s" ns" proc-read ;
: task-regs ( task -- c-addr u ) s" regs" proc-read ;      \ PC=... A=... X=... Y=... S=... P=... RAM=.. ROM=..
: task-mem ( task addr c-addr u -- )                     \ u bytes of its memory from addr
  2>r >r s" mem" r> 0 proc-open 2r> proc-get ;
: task-ram ( task bank offset c-addr u -- )              \ u bytes of its bank from offset
  2>r >r 8192 um* swap r> + swap 2>r s" ram" 2r> proc-open 2r> proc-get ;
