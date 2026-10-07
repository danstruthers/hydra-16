\ profile.fs - what forth -l (a login shell: init's or wstart's, as /lib/shell names it) runs as it starts, after its
\ namespace (newns) and startup.fs, before its first prompt: what /rom/lib/profile does for rc.  Through the /lib
\ union, a /lib/forth/profile.fs on the RAM disk or a card takes the place of this one (the ROM's).  $window is its
\ window (init's, wstart's): its console goes at /dev, and its notes (Ctrl-C) to forth's note group.

require shell.fl       \ The shell: an rc command line at the prompt; cd bind mount unmount newns, the prompt ...

s" window" getenv 1 = swap c@ char 0 = and 0= [if]     \ (A window but 0)
unmount #c /dev
bind -a #c$window /dev
[then]
s" /dev/consctl" w/o open-file throw  dup s" group" rot write-file throw  close-file throw
