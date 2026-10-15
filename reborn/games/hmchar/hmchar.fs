#!/bin/forth
\ hmchar.fs - a HarnMaster character made, as the book's Character article makes one (lib harn: /lib/forth/harn.fs),
\ and written to a file (harnfile.c's text) for a game, Dingus Quest Zero's or any other, to read: C's hmchar
\ (games/hmchar) in HyForth, the same questions, the same profile, the same file; and from the same seed the same
\ character, its dice rolled in the same order.
\
\ hmchar.fs [-s seed] [-o file] [-q]
\   -s seed    the same character each time
\   -o file    where it's written (default: NAME.hm, in the current directory, NAME the character's)
\   -q         no questions: the first roll kept, an occupation, weapons and option points chosen at random, a
\              name made up
\
\ The steps (Character 1): the attributes rolled (kept, or rolled again); an occupation, its skills opened as the
\ Occupational Skills table has them (Character 15; a few of the book's, those of use in a dungeon; "three weapons at
\ OML + SB" chosen); the five option points spent (Character 16: a skill opened at its OML for one, or improved by its
\ SB for one, a second time for two more); a name.  Not here: the family's occupation and its skills, contacts, the
\ veterans' points.

require hydra.fl
require harn.fs

false value quiet
-1 value occ
create name 24 allot   0 value name-n
create out 64 allot    0 value out-n

: n. ( n -- ) 0 .r ;
: named ( c-addr u n c-addr2 u2 -- ) 2>r >r type space r> n. 2r> type ;

\ A line typed, trimmed (the input's end: the end, nothing written)
create in 82 allot
: ask ( c-addr u -- c-addr2 u2 )
  type  in 80 0 read-line throw 0= if s" " 1 write-line drop  1 0 sys-exits then  in swap -trailing ;

\ ---- The occupations (Character 15): each skill and its ML's multiple of SB (0: the skill's own OML); 128 + n, three
\ weapons at OML + SB times n; 255 the end
: occ-name ( i -- c-addr u ) >r s" Gladiator|Hunter/Trapper|Thief|Physician|Miner|Sage" r> |field ;
create occ-years 4 c, 5 c, 3 c, 7 c, 6 c, 5 c,
create occ-skills
  31 c, 6 c,  130 c, 0 c,  32 c, 6 c,  78 c, 2 c,  255 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,
  87 c, 5 c,  129 c, 0 c,  7 c, 4 c,  61 c, 4 c,  83 c, 4 c,  65 c, 3 c,  66 c, 3 c,  59 c, 2 c,  255 c, 0 c,
  5 c, 4 c,  11 c, 5 c,  7 c, 4 c,  12 c, 4 c,  70 c, 3 c,  37 c, 5 c,  38 c, 4 c,  0 c, 3 c,  255 c, 0 c,
  78 c, 4 c,  64 c, 3 c,  49 c, 3 c,  20 c, 0 c,  255 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,
  76 c, 4 c,  57 c, 3 c,  90 c, 3 c,  74 c, 2 c,  68 c, 1 c,  255 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,
  60 c, 5 c,  73 c, 2 c,  20 c, 0 c,  255 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,  0 c, 0 c,
\ (Gladiator: Initiative/6, three weapons at OML + SB x 2, Unarmed/6, Physician/2.  Hunter/Trapper: Tracking/5,
\ weapons at OML + SB, Stealth/4, Foraging/4, Survival/4, Hidework/3, Hunting/3, Fletching/2.  Thief: Legerdemain/4,
\ Awareness/5, Stealth/4, Intrigue/4, Lockcraft/3, Club/5, Dagger/4, Acrobatics/3.  Physician: Physician/4,
\ Herblore/3, Alchemy/3, Script.  Miner: Mining/4, Engineering/3, Woodcraft/3, Metalcraft/2, Jewelcraft/1.  Sage:
\ Folklore/5, Mathematics/2, Script)
create weapons 34 c, 36 c, 37 c, 38 c, 39 c, 40 c, 41 c, 42 c, 43 c, 44 c, 45 c, 46 c, 35 c,
\ (Axe, Bow, Club, Dagger, Flail, Net, Polearm, Shield, Sling, Spear, Sword, Whip, Blowgun)

\ A skill named (case ignored, a beginning of its name enough if only one has it), among n (addr: their numbers, a
\ byte each; 0: skill 0 on): it, -2 (more than one begins so) or -1 (none), as C's skill_named
: lower-c ( c -- c' ) dup [char] A [char] Z 1+ within if 32 + then ;
: begins? ( c-addr u c-addr2 u2 -- flag )            \ The second begins with the first, case ignored
  2 pick < if 2drop drop false exit then
  swap 0 ?do over i + c@ lower-c over i + c@ lower-c <> if 2drop false unloop exit then loop 2drop true ;
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

: chosen? ( sk -- flag ) dup harn-ml@ swap harn-oml > ;
variable times   variable got
: three-weapons ( n -- )
  times !
  quiet 0= if
    ." Three weapon skills, opened at their OML plus " times @ 1 = 0= if ." twice " then ." SB. The weapons:"
    13 0 do i if ." ," then space weapons i + c@ harn-skill-name type loop ." ." cr
  then
  0 got !  begin got @ 3 < while
    quiet if weapons 13 random + c@ else
      got @ dup 0= if drop s" The first: " else 1 = if s" The second: " else s" The third: " then then
      ask weapons 13 skill-named
    then
    dup 0< if
      quiet 0= if -2 = if ." Which? More than one begins so." else ." That's not one of them." then cr else drop then
    else dup chosen? if quiet 0= if ." That one's chosen already." cr then drop
    else dup harn-sb times @ * over harn-oml + swap harn-ml!  1 got +! then then
  repeat ;

\ The occupation's skills: an automatic skill listed takes the occupation's ML, not the two added (Character 16)
: occupation ( -- )
  quiet if 6 random to occ else
    cr ." An occupation (Character 15):" cr
    6 0 do ."   " i 1+ n. space i occ-name type s" ," i occ-years + c@ s"  years" named cr loop
    begin occ 0 6 within 0= while
      s" Which (1-6)? " ask 0 0 2swap >number 2drop drop 1- to occ
    repeat
  then
  occ 18 * occ-skills +  9 0 do
    dup c@ dup 255 = if drop leave then
    dup 128 and if 127 and three-weapons else
      over 1+ c@ ?dup if over harn-sb * else dup harn-oml then swap harn-ml!
    then 2 +
  loop drop ;

\ The skills listed (? at the option points' prompt): the open ones at their ML; the others (but the rituals: the
\ character has no deity) at the OML they'd open at; three to a line
variable col
: skill. ( sk v -- )
  >r  2 spaces harn-skill-name dup >r type  20 r> - 3 - 0 max spaces  r> 3 .r
  1 col +!  col @ 3 = if cr 0 col ! then ;
: list-skills ( -- )
  ." Your skills, at their ML:" cr  0 col !
  91 0 do i harn-ml@ ?dup if i swap skill. then loop  col @ if cr then
  ." The others, at the OML they would open at:" cr  0 col !
  91 0 do i harn-ml@ 0= i 21 31 within 0= and if i dup harn-oml skill. then loop  col @ if cr then ;

\ Five option points: a skill opened at OML for one; an open skill improved by its SB for one (Character 16); no more
\ than two points in a skill (the user's: the book's second improvement, two more, would make three)
create improved 91 allot   variable op   variable cost
: option-points ( -- )
  improved 91 erase  5 op !
  quiet 0= if
    cr ." Five option points: a skill's name opens it at its OML (1 point), or improves an open one by its SB" cr
    ." (1 point), no more than two points in a skill. ? lists them. Enter alone keeps what's left." cr
  then
  begin op @ while
    quiet if 91 random else
      s" Points left:" op @ s" . A skill: " named  s" " ask  dup 0= if 2drop exit then
      2dup s" ?" compare 0= if 2drop list-skills -3 else 0 91 skill-named then
    then
    dup 0< if
      quiet 0= if dup -3 = if drop else -2 = if ." Which? More than one begins so." else ." There's no such skill."
      then cr then else drop then
    else quiet over 21 31 within and if drop        \ (Not a ritual: the character has no deity)
    else
      dup harn-ml@ 0= over improved + c@ 0= or if 1 else 255 then cost !
      cost @ op @ > if
        quiet 0= if
          cost @ 255 = if ." No more than two points in a skill: that one's had its share."
          else ." Not enough points left for that." then cr
        then
        drop
      else
        dup harn-ml@ 0= if dup harn-open else
          dup harn-ml@ over harn-sb + over harn-ml!  dup improved + dup c@ 1+ swap c! then
        cost @ negate op +!
        quiet 0= if dup harn-skill-name type dup harn-ml@ >r s"  is" r> s" " named cr then drop
      then
    then then
  repeat ;

\ ---- The profile
: profile ( -- )
  cr name-n if name name-n type ." , " then
  harn-sp harn@ harn-species-name type space harn-sx harn@ harn-sex-name type ." , born "
  harn-bd harn@ 255 and n. space harn-bd harn@ 8 rshift harn-month-name type ."  ("
  harn-ss harn@ harn-sunsign-name type ." ); " harn-culture c@ harn-culture-name type space
  harn-social c@ harn-class-name type  occ 0< 0= if ." , " occ occ-name type then cr
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
: usage ( -- ) s" usage: hmchar.fs [-s seed] [-o file] [-q]" 2 write-line drop  1 0 sys-exits ;
variable ai
: option ( -- )                                   \ The option at ai, ai past it (and its value)
  ai @ arg
  2dup s" -q" word= if 2drop true to quiet  1 ai +! exit then
  ai @ 1+ argc < 0= if 2drop usage then
  2dup s" -s" word= if 2drop ai @ 1+ arg 0 0 2swap >number 2drop rseed  2 ai +! exit then
  s" -o" word= if ai @ 1+ arg 63 min dup to out-n out swap move  2 ai +! exit then
  usage ;
: options ( -- ) 1 ai !  begin ai @ argc < while option repeat ;
: hmchar ( -- )
  options
  begin harn-create  quiet if true else
    profile cr s" Keep this character, or roll again (k or r)? " ask if c@ lower-c [char] k = else drop false then
  then until
  occupation  option-points
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
  line{ s" occupation " line+ occ occ-name line+ line} drop
  close-file drop
  ." Written to " out out-n type cr ;
hmchar
