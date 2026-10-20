#!/bin/forth
\ hmchar.fs - a HarnMaster character made, as the book's Character article makes one (lib harn: /lib/forth/harn.fs),
\ and written to a file (harnfile.c's text) for a game, Dingus Quest Zero's or any other, to read: C's hmchar
\ (games/hmchar) in HyForth, the same questions, the same profile, the same file; and from the same seed the same
\ character, its dice rolled in the same order.
\
\ hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation]
\   -s seed    the same character each time
\   -o file    where it's written (default: NAME.hm, in the current directory, NAME the character's)
\   -q         no questions: the first roll kept, an occupation (one of the book's), weapons and option points chosen
\              at random, a name made up
\   -p species the character's species, not rolled: human, elf (Sindarin) or dwarf (Khuzdul), by its first letter
\              (h, e or s, d or k)
\   -c occupation  the occupation, by its name (case ignored, a beginning enough), not asked or drawn (-q's draw made
\              all the same: the rest of the character as it would be)
\
\ The steps (Character 1): the attributes rolled (kept, or rolled again); an occupation, any of the book's that
\ occupations.txt has (C's hmchar's, its lib/hmchar: the Occupational Skills table, Character 15; a soldier's unit by
\ the character's culture, Character 27; a cleric's church by its deity, Character 23; not yet a mage's, Character
\ 25), its skills opened as the book has them (the weapons or crafts it gives chosen); or the player's own (11
\ points, two in a skill at most and three in one, its primary); the five option points spent (Character 16: a
\ skill opened at its OML for one, or improved by its SB for one, two points in a skill at most); a name.  Not here:
\ the family's occupation and its skills, contacts, the veterans' points.

require hydra.fl
require harn.fs

false value quiet
create name 24 allot   0 value name-n
create out 64 allot    0 value out-n
create occname 20 allot  0 value occ-n          \ The occupation (its name; the player's own, as it's called)
create cname 24 allot  0 value cname-n          \ -c's

: n. ( n -- ) 0 .r ;
: named ( c-addr u n c-addr2 u2 -- ) 2>r >r type space r> n. 2r> type ;

\ The column the output's at (the occupation's words wrapped as C's are: its ch's)
variable col
: say ( c-addr u -- ) dup col +! type ;
: nl ( -- ) cr 0 col ! ;
: sayn ( n -- ) 0 <# #s #> say ;
: w ( c-addr u -- )                               \ After a space, or on a line of its own (indented) past column 78
  col @ over + 77 > if nl s"   " else s"  " then say say ;

\ A line typed, trimmed (the input's end: the end, nothing written)
create in 82 allot
: ask ( c-addr u -- c-addr2 u2 )
  type  in 80 0 read-line throw 0= if s" " 1 write-line drop  1 0 sys-exits then  in swap -trailing  0 col ! ;

create weapons 34 c, 36 c, 37 c, 38 c, 39 c, 40 c, 41 c, 42 c, 43 c, 44 c, 45 c, 46 c, 35 c,
\ (Axe, Bow, Club, Dagger, Flail, Net, Polearm, Shield, Sling, Spear, Sword, Whip, Blowgun)
: crafts, ( -- ) 91 48 do i c, loop ;
create crafts crafts,                             \ (Agriculture ... Woodcraft: 43)

\ A skill named (case ignored, a beginning of its name enough if only one has it), among n (addr: their numbers, a
\ byte each; 0: skill 0 on): it, -2 (more than one begins so) or -1 (none), as C's skill_named
: lower-c ( c -- c' ) dup [char] A [char] Z 1+ within if 32 + then ;
: begins? ( c-addr u c-addr2 u2 -- flag )            \ The second begins with the first, case ignored
  2 pick < if 2drop drop false exit then
  swap 0 ?do over i + c@ lower-c over i + c@ lower-c <> if 2drop false unloop exit then loop 2drop true ;
: same? ( c-addr u c-addr2 u2 -- flag ) 2 pick over <> if 2drop 2drop false else begins? then ;
create typed 2 cells allot   variable found
: skill-named ( c-addr u addr n -- sk )
  2swap typed 2!  typed @ 0= if 2drop -1 exit then
  -1 found !  0 ?do
    dup if dup i + c@ else i then             ( addr sk )
    typed 2@ 2 pick harn-skill-name begins? if
      dup harn-skill-name nip typed @ = if nip unloop exit then
      found @ 0< 0= if 2drop -2 unloop exit then  found !
    else drop then
  loop drop found @ ;

\ ---- The occupations: occupations.txt (lib/hmchar in hmchar.fs's own directory, as rc's $prog has it, else the
\ card's games/lib/hmchar: C's hmchar's), a line each in groups by kind; read a line at a time each time it's wanted,
\ as C's hmchar (its bank program hmocc) reads it
create libdir 64 allot  0 value libdir-n
create pbuf 80 allot  variable pn
: p+ ( c-addr u -- ) tuck pbuf pn @ + swap move pn +! ;
: occ-file ( -- c-addr u ) 0 pn !  libdir libdir-n p+ s" /occupations.txt" p+ pbuf pn @ ;
: lib+ ( c-addr u -- ) tuck libdir libdir-n + swap move libdir-n + to libdir-n ;
: find-lib ( -- )
  0 to libdir-n
  s" /env/prog" r/o open-file 0= if
    >r libdir 30 r@ read-file 0= if to libdir-n else drop then  r> close-file drop
  else drop then
  libdir-n begin dup 0> while dup 1- libdir + c@ [char] / = if to libdir-n s" lib/hmchar" lib+ exit then 1- repeat
  to libdir-n s" lib/hmchar" lib+ ;
: lib-ok? ( -- flag ) occ-file r/o open-file if drop false else close-file drop true then ;

0 value lfid
create lbuf 258 allot
create cur 2 cells allot                          \ (The line's rest)
: line? ( -- flag )                               \ The file's next line in cur (its CR gone): false at its end
  lbuf 256 lfid read-line throw  if lbuf swap  dup if 2dup + 1- c@ 13 = if 1- then then  cur 2! true
  else drop false then ;
variable f-a  variable f-u
: fld ( -- c-addr u c )                           \ The line's next field, its spaces trimmed; what ended it (| , or 0)
  cur 2@ begin dup if over c@ bl = else false then while 1 /string repeat
  over f-a !
  begin dup if over c@ dup [char] | = swap [char] , = or 0= else false then while 1 /string repeat
  over f-a @ - f-u !
  dup if over c@ >r 1 /string r> else 0 then  >r cur 2! r>
  f-a @ f-u @ -trailing rot ;

create kind 12 allot  0 value kind-n              \ The group's kind ("soldier")
0 value sect   10 value sdty                      \ The group offered, its deity (a church's; else none: 10)
\ A group's header, its [ passed: its kind, whether it's offered (no qualifier, or the character's culture, or its
\ deity, or a deity when it has none), its deity: true if a title follows (the line's rest, cur)
create qual 2 cells allot
: header ( -- title? )
  cur 2@ over >r begin dup if over c@ [char] ] <> else false then while 1 /string repeat  ( a' u' )
  over r@ - r> swap 2swap  dup if 1 /string then cur 2!   ( start len )
  2dup s"  " search if 2swap 2 pick - 11 min  2swap 1 /string 11 min else 2drop 11 min 0 0 then  qual 2!
  dup to kind-n kind swap move
  10 to sdty  10 0 do qual 2@ i harn-deity-name same? if i to sdty then loop
  qual 2@ nip 0=  qual 2@ harn-culture c@ harn-culture-name same? or
  sdty 10 <> harn-dty harn@ dup 10 = swap sdty = or and or to sect
  cur 2@ if c@ bl = else drop false then  dup if cur 2@ 1 /string cur 2! then ;
\ The title said (a long one broken at a space past the 68th column)
: title ( -- )
  cur 2@ 0 ?do dup i + c@ dup bl = col @ 68 > and if drop nl s"   " say else 1 col +! emit then loop drop ;

0 constant o-count   1 constant o-list   2 constant o-find   3 constant o-take
0 value want   0 value take   0 value talk   0 value cell#
0 value occk   0 value occkind   10 value occdty
create wname 2 cells allot
0 value pend-n   0 value pend-m   0 value pend-w

: brk ( -- ) col @ if nl then 0 to cell# ;
: upto ( first last -- ) tuck <> if s" -" say sayn else drop then nl ;

\ A skill of an occupation's: SKILL/N, SKILL (at its OML), Ritual/N (the deity's), or weapons N/M or crafts N/M (chosen
\ after: pend-*); a skill listed twice, the higher
: num ( c-addr u -- n ) 0 0 2swap >number 2drop drop ;
: skill-token ( c-addr u -- )
  2dup s" /" search if over >r 1 /string num nip r> 2 pick - swap else 2drop 0 then   ( c-addr u' m )
  >r
  2dup s" weapons " 2swap begins? >r 2dup s" crafts " 2swap begins? r> or if
    over c@ lower-c [char] w = dup to pend-w  7 swap - /string num to pend-n  r> to pend-m exit
  then
  2dup s" Ritual" same? if 2drop harn-dty harn@ dup 10 = if drop r> drop exit then 21 +
  else 2dup 0 harn-nskills skill-named dup 0< if drop
      s" hmchar.fs: occupations.txt: no skill " 2 write-file drop 2 write-line drop r> drop exit then nip nip
  then
  r> ?dup if over harn-sb * else dup harn-oml then   ( sk v )
  over harn-ml@ over < if swap harn-ml! else 2drop then ;

\ The occupations offered, each in turn, by mode (o-count ...): o-count's how many there are (not the player's own),
\ o-list's how many it listed, o-find's how many it found (2: more than one begins so)
0 value k   0 value got   0 value first   0 value last   0 value exact   0 value all   0 value say?   0 value apply
create nm 2 cells allot   create yr 2 cells allot
: offered ( mode -- )                             \ An offered line, its name and years read
  case
    o-count of all 0= talk and kind c@ last <> and if
        last if first k 1- upto then  s"   " say kind kind-n say s"  " say k sayn  k to first  kind c@ to last
      then endof
    o-list of kind c@ want = if
        all if brk true to say? else
          cell# 3 = if brk then  begin col @ cell# 26 * < while s"  " say repeat
          k 10 < if s"    " else s"   " then say  k sayn s"  " say nm 2@ say  cell# 1+ to cell#  got 1+ to got
        then
      then endof
    o-find of all 0= if
        want if k want = else exact 0= wname 2@ nm 2@ begins? and then if
          want 0= nm 2@ nip wname @ = and to exact
          exact got 0= or if
            k to occk kind c@ to occkind sdty to occdty nm 2@ 19 min dup to occ-n occname swap move
          then
          got 1+ to got  exact if 1 to got then
        then
      then endof
    o-take of kind c@ occkind = all nm 2@ occname occ-n compare 0= or and if
        quiet 0= to say?  take to apply
      then endof
  endcase
  say? if all if s" Every " say kind kind-n say s" :" say else nm 2@ say s" , " say yr 2@ say s"  years:" say then then ;
: scan ( mode -- n )
  >r 0 to k  0 to got  0 to first  0 to last  0 to exact
  occ-file r/o open-file if r> 2drop 0 exit then to lfid
  begin line? while
    cur 2@ if c@ else drop 0 then
    dup [char] [ = if drop cur 2@ 1 /string cur 2!
      header r@ o-list = and sect and kind c@ want = and if brk title nl then
    else dup [char] ; = swap 0= or 0= if
      fld >r nm 2! r> dup [char] | = if drop fld >r yr 2! r> then
      nm 2@ s" all" same? to all  0 to say?  0 to apply
      sect nm @ 0<> and if all 0= if k 1+ to k then  r@ offered then
      begin dup [char] | = over [char] , = or while drop
        fld >r say? if 2dup w r@ [char] , = if s" ," say then then  apply if skill-token else 2drop then r>
      repeat drop
      say? if nl then
    then then
  repeat
  lfid close-file drop
  r> case
    o-count of talk if last if first k upto then s"   your own" k 1+ s" " named nl then k endof
    o-list of brk got endof
    drop got 0
  endcase ;

\ ---- The weapons or crafts an occupation gives the character to choose: pend-n weapons (pend-w) or crafts, each
\ opened at its OML plus SB times pend-m (a weapon), or at SB times pend-m (a craft)
: count-word ( n -- c-addr u ) >r s" One|Two|Three" r> 1- |field ;
: times-word ( n -- c-addr u ) >r s" ||twice |three times " r> |field ;
variable among   variable among-n   variable got#
: choose ( -- )
  pend-w if weapons 13 else crafts 43 then among-n ! among !
  quiet 0= if
    pend-n count-word say  pend-w if s"  weapon skill" else s"  craft" then say
    pend-n 1 > if s" s, opened at " else s" , opened at " then say
    pend-w if s" their OML" say pend-m if s"  plus " say then then
    pend-m pend-w 0= or if pend-m times-word say pend-w if s" SB" else s" their SB" then say then
    pend-w if s" . The weapons:" else s" . The crafts:" then say
    among-n @ 0 do among @ i + c@ harn-skill-name w  i 1+ among-n @ < if s" ," else s" ." then say loop nl
  then
  0 got# !  begin got# @ pend-n < while
    quiet if among @ among-n @ random + c@ else
      got# @ dup 0= if drop s" The first: " else 1 = if s" The second: " else s" The third: " then then
      ask among @ among-n @ skill-named
    then
    dup 0< if
      quiet 0= if -2 = if s" Which? More than one begins so." else s" That's not one of them." then type cr else drop then
    else
      dup harn-sb pend-m * over harn-oml pend-w and +       ( sk v )
      over harn-ml@ over < 0= if 2drop quiet 0= if ." That one's chosen already." cr then
      else swap harn-ml!  1 got# +! then
    then
  repeat ;

\ ---- The skills listed (? at the points' prompt): the open ones at their ML; the others (but the rituals: the
\ character has no deity, or its own's open) at the OML they'd open at; three to a line
variable lcol
: skill. ( sk v -- )
  >r  2 spaces harn-skill-name dup >r type  20 r> - 3 - 0 max spaces  r> 3 .r
  1 lcol +!  lcol @ 3 = if cr 0 lcol ! then ;
: list-skills ( -- )
  ." Your skills, at their ML:" cr  0 lcol !
  91 0 do i harn-ml@ ?dup if i swap skill. then loop  lcol @ if cr then
  ." The others, at the OML they would open at:" cr  0 lcol !
  91 0 do i harn-ml@ 0= i 21 31 within 0= and if i dup harn-oml skill. then loop  lcol @ if cr then ;

\ Points spent, op of them (Character 16's option points; or own, the player's own occupation's): a skill's name opens
\ it at its OML for one, or improves an open one by its SB for one; no more than two points in a skill (an option
\ point's: an open skill has had one, its opening), but three in one skill, the player's own occupation's primary
create pts 91 allot   variable op   0 value own   0 value primary
: spend ( op own -- )
  to own op !  own 0= to primary
  91 0 do own 0= i harn-ml@ 0<> and 1 and pts i + c! loop
  quiet 0= if own if
      cr ." Your own occupation, 11 points: a skill's name opens it at its OML (1 point)," cr
      ." or improves an open one by its SB (1 point); two points in a skill at most, but" cr
      ." three in one, its primary. ? lists them. Enter alone keeps what's left." cr
    else
      cr ." Five option points: a skill's name opens it at its OML (1 point), or improves an open one by its SB" cr
      ." (1 point), no more than two points in a skill. ? lists them. Enter alone keeps what's left." cr
  then then
  begin op @ while
    quiet if 91 random else
      s" Points left:" op @ s" . A skill: " named  s" " ask  dup 0= if 2drop exit then
      2dup s" ?" compare 0= if 2drop list-skills -3 else 0 91 skill-named then
    then
    dup 0< if
      quiet 0= if dup -3 = if drop else -2 = if ." Which? More than one begins so." else ." There's no such skill."
      then cr then else drop then
    else dup 21 31 within over 21 - harn-dty harn@ <> quiet or and if
      drop quiet 0= if ." Not your deity's ritual." cr then     \ (-q's draws none: its dice as they were)
    else dup pts + c@ dup 2 < 0= swap 2 > primary or and if
      drop quiet 0= if ." No more than two points in a skill" own if ." , or three in one" then
        ." : that one's had its share." cr then
    else
      dup pts + c@ 2 = if true to primary then  dup pts + dup c@ 1+ swap c!
      dup harn-ml@ 0= if dup harn-open else dup harn-ml@ over harn-sb + over harn-ml! then
      -1 op +!
      quiet 0= if dup harn-skill-name type dup harn-ml@ >r s"  is" r> s" " named cr then drop
    then then then
  repeat ;

\ ---- The occupation: one of the book's, chosen (by its number or its name, a group listed by its kind's first
\ letter, ? and one its skills shown), or -c's, or -q's at random (one draw, -c or not); its skills opened (an
\ automatic skill listed takes the occupation's OML, not the two added: Character 16); or the player's own
0 value occs#   0 value show
: no-occ ( -- ) s" hmchar.fs: no lib/hmchar/occupations.txt" 2 write-line drop  1 0 sys-exits ;
: occupation ( -- )
  cname cname-n wname 2!  quiet 0= cname-n 0= and to talk
  talk if cr ." An occupation (Character 15; not yet a mage's, Character 25: theirs come with magic):" cr then
  o-count scan dup to occs# 0= if no-occ then
  0 to want  quiet if occs# random 1+ to want then
  cname-n if
    0 to want  o-find scan 1 <> if
      s" hmchar.fs: -c: no such occupation, or more than one begins so" 2 write-line drop  1 0 sys-exits
    then
  else quiet if o-find scan drop else
    ." A group's first letter lists it; a number or a name chooses one (a beginning" cr
    ." enough), and ? before one shows its skills." cr
    begin
      s" Which? " ask  dup if over c@ [char] ? = else false then dup to show if 1 /string then
      begin dup if over c@ bl = else false then while 1 /string repeat     ( c-addr u )
      dup 0= if 2drop false else
        over c@ lower-c to want
        show 0= over 1 = and want [char] a [char] z 1+ within and if o-list scan else 0 then if 2drop false else
          1 to got  0 to occk
          over c@ [char] 0 [char] 9 1+ within if
            num dup occs# 1+ <> if dup to want  dup 0> swap occs# > 0= and if o-find scan else 0 then to got
            else drop then
          else 2dup s" your own" begins? if 2drop else wname 2! 0 to want  o-find scan to got then then
          got 1 <> if got if ." Which? More than one begins so." else ." There's no such occupation." then cr false
          else show 0= dup 0= if
              occk if 0 to take  o-take scan drop
              else ." Your own: 11 points spent as option points are, three in one skill (its primary)." cr then
          then then
        then
      then
    until
  then then
  occk 0= if
    11 true spend  0 to occ-n
    begin occ-n 0= while cr s" Your occupation's name: " ask 19 min dup to occ-n occname swap move repeat exit
  then
  occdty 10 <> harn-dty harn@ 10 = and if occdty harn-dty harn!  occdty 21 + harn-open then
  1 to take  0 to pend-n  o-take scan drop
  pend-n if choose then ;

\ ---- The profile
: profile ( -- )
  cr name-n if name name-n type ." , " then
  harn-sp harn@ harn-species-name type space harn-sx harn@ harn-sex-name type ." , born "
  harn-bd harn@ 255 and n. space harn-bd harn@ 8 rshift harn-month-name type ."  ("
  harn-ss harn@ harn-sunsign-name type ." ); " harn-culture c@ harn-culture-name type space
  harn-social c@ harn-class-name type  occ-n if ." , " occname occ-n type then cr
  harn-sibrank c@ n. ."  of " harn-famsize c@ n. ." ; " harn-parent c@ harn-parent-name type ." , "
  harn-estrange c@ harn-estrangement-name type ." ; clanhead: " harn-clanhead c@ harn-clanhead-name type cr
  harn-ht harn@ n. [char] " emit ." , " harn-fr harn@ harn-frame-name type ." , " harn-wt harn@ n. ."  lb (size "
  harn-size n. ." ); " harn-cml harn@ harn-comeliness-name harn-cml harn@ s" ; " named
  harn-complexion c@ harn-complexion-name type ." , " harn-hair c@ harn-hair-name type ."  hair, "
  harn-eyes c@ harn-eye-name type ."  eyes" cr
  harn-mor 1+ harn-str do
    i harn-attr-name i harn@ i harn-voi = i harn-mor = or if s" " else s" , " then named  i harn-voi = if cr then
  loop cr
  s" Endurance" harn-end s" , " named  s" Move" harn-move s" , " named  s" Piety" harn-pty harn@ s" ; medical:" named
  0 harn-med c@ if 4 0 do i harn-med c@ ?dup if space harn-medical-name type then loop else ."  none" then cr
  ." Skills (ML):"  0  harn-nskills 0 do        \ (A line 78 at most, as C's: the column)
    i harn-ml@ if
      i harn-skill-name nip 5 +  2dup + 78 > if nip cr space 1 swap then +
      space i harn-skill-name type space i harn-ml@ n.
    then
  loop drop cr ;

\ ---- Names (-q's): two syllables of a few, Harnic-sounding enough
: name+ ( c-addr u -- ) tuck name name-n + swap move name-n + to name-n ;
: make-name ( -- )
  0 to name-n  s" Al|Bra|Dor|Ka|Ly|Mo|Ne|Ro|Sa|Tor|Wy|Ha" 12 random |field name+
  s" dan|ric|nel|gyn|wen|lor|vik|mar|seth|ka|ven" 11 random |field name+ ;

\ ---- The program
: err ( c-addr u -- ) 2 write-file drop ;
: usage ( -- ) s" usage: hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation]" 2 write-line drop  1 0 sys-exits ;
: species-of ( c-addr u -- sp )                   \ -p's: h a human, e or s an elf, d or k a dwarf: 0-2, or -1
  0= if drop -1 exit then  c@ lower-c
  5 0 do s" hesdk" drop i + c@ over = if drop i 1+ 2/ unloop exit then loop  drop -1 ;
variable ai
: option ( -- )                                   \ The option at ai, ai past it (and its value)
  ai @ arg
  2dup s" -q" word= if 2drop true to quiet  1 ai +! exit then
  ai @ 1+ argc < 0= if 2drop usage then
  2dup s" -s" word= if 2drop ai @ 1+ arg 0 0 2swap >number 2drop rseed  2 ai +! exit then
  2dup s" -p" word= if 2drop ai @ 1+ arg species-of dup 0< if usage then harn-species !  2 ai +! exit then
  2dup s" -c" word= if 2drop ai @ 1+ arg 23 min dup to cname-n cname swap move  2 ai +! exit then
  s" -o" word= if ai @ 1+ arg 63 min dup to out-n out swap move  2 ai +! exit then
  usage ;
: options ( -- ) 1 ai !  begin ai @ argc < while option repeat ;
: hmchar ( -- )
  options
  find-lib  lib-ok? 0= if 0 to libdir-n s" /sd/0/games/lib/hmchar" lib+ then
  begin harn-create  quiet if true else
    profile cr s" Keep this character, or roll again (k or r)? " ask if c@ lower-c [char] k = else drop false then
  then until
  occupation  5 false spend
  quiet if make-name else
    begin name-n 0= while cr s" A name: " ask 23 min dup to name-n name swap move repeat
  then
  profile
  out-n 0= if name name-n out swap move  s" .hm" out name-n + swap move  name-n 3 + to out-n then
  out out-n w/o create-file if
    drop s" hmchar.fs: " err out out-n err s" : can't write it" 2 write-line drop  1 0 sys-exits
  then
  dup harn-write drop
  line{ s" name " line+ name name-n line+ line} drop
  line{ s" occupation " line+ occname occ-n line+ line} drop
  close-file drop
  ." Written to " out out-n type cr ;
hmchar
