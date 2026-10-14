\ harn.fs - HarnMaster (third edition, 3.5) characters and their rules: lib harn.  danlang's lib/harn.dl and the C
\ SDK's harn.h in HyForth: harn.h's numbers (a species 0-3, a sunsign 0-35 ...), its dice in harn_create's order (so a
\ seed, rseed's, makes the character C's harn_create makes from it), and its file (harnfile.c's lines), which the C
\ game and every generator read.  The book's parts: Character 2-12 (a character made), Skills 2-6 and 21-22 (skills,
\ tests, penalties, development).  Combat and healing are harn.dl's and harn.h's; occupations, contacts, aging,
\ psionics, magic and rituals are left out, as there.
\
\ A character is a record of /harn bytes; harn-c is the one the words work on (harn-c0 at the start: to harn-c for
\ another).  Its attributes, by harn.h's numbers (harn-sp ... harn-pty), a cell each: harn@ ( a -- n ), harn! ( n a --
\ ); its bytes: harn-culture harn-social harn-sibrank harn-famsize harn-parent harn-estrange harn-clanhead,
\ harn-complexion harn-hair harn-eyes, harn-med ( i -- addr ) (its traits, 0 an empty place), harn-psy ( i -- addr )
\ and harn-psysev, harn-sexuality, harn-ml ( sk -- addr ) (a skill's mastery level, 0 not opened), the volatiles
\ harn-fl harn-bl harn-dp harn-state harn-shockhr.  harn-load is what it carries, in pounds.  Injuries aren't kept.
\
\   harn-create ( -- )             a character made at random (harn.h's harn_create)
\   harn-sb harn-oml ( sk -- n )   a skill's base and opening mastery; harn-open ( sk -- ) opens it
\   harn-end harn-size harn-move ( -- n )   endurance, size (1-10), move
\   harn-test ( eml -- level )     d100 against an EML: 0 CF, 1 MF, 2 MS, 3 CS; harn-eml ( sk -- eml ) and
\                                  harn-test-skill ( sk mod -- level ); harn-develop ( sk -- r ): 0 improved,
\                                  1 not improved, 2 maxed
\   harn-write ( fileid -- ior )   the character as harnfile.c writes it; harn-write-line ( c-addr u fileid -- ior )
\                                  a line of the program's own after it
\   harn-read ( fileid -- ior )    one read back; each line not harn.h's given to harn-other ( c-addr u -- )
\   the names: harn-species-name ( i -- c-addr u ) ... harn-skill-name ( sk -- c-addr u ), harn-skill-key

require random.fl
require string.fl

\ ---- Dice: random's numbers (xorshift32, as dice.h's and hylang's), random's way to a die (the high cell of rand * n)
: die ( n -- 1..n ) random 1+ ;
: dice ( n sides -- sum ) 0 rot 0 ?do over die + loop nip ;
: d100 ( -- 1..100 ) 100 die ;

\ A table of n upper bounds (cells): the first a roll doesn't pass (the last if it passes them all)
: htable ( r a n -- k )
  dup >r 1- 0 ?do  2dup @ > 0= if 2drop i unloop r> drop exit then  cell+  loop  2drop r> 1- ;

\ ---- The character
0 constant harn-sp     1 constant harn-sx     2 constant harn-bd     3 constant harn-ss     4 constant harn-ht
5 constant harn-fr     6 constant harn-owt    7 constant harn-wt     8 constant harn-cml    9 constant harn-str
10 constant harn-sta  11 constant harn-dex   12 constant harn-agl   13 constant harn-eye   14 constant harn-hrg
15 constant harn-sml  16 constant harn-voi   17 constant harn-int   18 constant harn-aur   19 constant harn-wil
20 constant harn-mor  21 constant harn-dty   22 constant harn-pty
23 constant harn-nattrs
\ (harn-bd is (month << 8) | day; harn-ss 3 times the sign, and 0 its first days' cusp, 1 the sign alone, 2 its last
\ days' cusp; harn-dty a deity, 10 none)

172 constant /harn
create harn-c0 /harn allot
harn-c0 value harn-c
: harn@ ( a -- n ) cells harn-c + @ ;
: harn! ( n a -- ) cells harn-c + ! ;
: harn-culture ( -- addr ) harn-c 46 + ;     : harn-social ( -- addr ) harn-c 47 + ;
: harn-sibrank ( -- addr ) harn-c 48 + ;     : harn-famsize ( -- addr ) harn-c 49 + ;
: harn-parent ( -- addr ) harn-c 50 + ;      : harn-estrange ( -- addr ) harn-c 51 + ;
: harn-clanhead ( -- addr ) harn-c 52 + ;
: harn-complexion ( -- addr ) harn-c 53 + ;  : harn-hair ( -- addr ) harn-c 54 + ;
: harn-eyes ( -- addr ) harn-c 55 + ;
: harn-med ( i -- addr ) harn-c 56 + + ;     : harn-psy ( i -- addr ) harn-c 60 + + ;
: harn-psysev ( i -- addr ) harn-c 63 + + ;  : harn-sexuality ( -- addr ) harn-c 66 + ;
: harn-ml ( sk -- addr ) harn-c 67 + + ;     \ (100 places: the book's 91 skills, a program's own after them)
: harn-fl ( -- addr ) harn-c 167 + ;         : harn-bl ( -- addr ) harn-c 168 + ;
: harn-dp ( -- addr ) harn-c 169 + ;         : harn-state ( -- addr ) harn-c 170 + ;
: harn-shockhr ( -- addr ) harn-c 171 + ;
variable harn-load   0 harn-load !

\ ---- The names: a list's fields, | between them; a list made over lines (list: name, then |" text" lines)
: |skip ( c-addr u -- c-addr' u' )
  begin dup 0= if exit then  over c@ [char] | <> while 1 /string repeat 1 /string ;
: |field ( c-addr u i -- c-addr' u' )
  0 ?do |skip loop  over swap 0 ?do dup c@ [char] | = if leave then 1+ loop  over - ;
variable list-at
: list: ( "name" -- ) create here list-at ! 0 , ;
: |" ( "text<quote>" -- ) [char] " parse  dup list-at @ +!  here swap dup allot move ;
: list ( addr -- c-addr u ) dup cell+ swap @ ;
list: attr-l
|" Species|Sex|Birthday|Sunsign|Height|Frame|Optimum Weight|Weight|Comeliness|Strength|Stamina|Dexterity|"
|" Agility|Eyesight|Hearing|Smell|Voice|Intelligence|Aura|Will|Morality|Deity|Piety|Endurance"
list: month-l
|" Nuzyael|Peonu|Kelen|Nolus|Larane|Agrazhar|Azura|Halane|Savor|Ilvin|Navek|Morgat"
list: sign-l
|" Ulandus|Aralius|Feniri|Ahnu|Angberelius|Nadai|Hirin|Tarael|Tai|Skorus|Masara|Lado"
list: medical-l
|" None|Addiction|Albinism|Allergy|Ambidextrous|Arm|Birthmark|Dwarfism|Epilepsy|Ear|Eye|Gigantism|"
|" Hemophilia|Hirsutism|Leg|Left-Handed|Leprosy|Lycanthropy|Monochromasia|Obesity|Parasites|Poxmarks|"
|" Scars|Sterile"
list: psyche-l
|" None|Acrophobia|Aichmophobia|Algophobia|Astraphobia|Ballistophobia|Bulimia|Claustrophobia|Cynophobia|"
|" Dyslexia|Entomophobia|Gamblamania|Gephyrophobia|Heliophobia|Hemophobia|Hydrophobia|Hypochondria|"
|" Ivashuphobia|Kleptomania|Megalomania|Mysophobia|Multiple Personality|Musophobia|Necrophobia|"
|" Nyctophobia|Ophidiophobia|Ornithophobia|Paranoia|Psychopathy|Pvaraphobia|Pyromania|Pyrophobia|"
|" Sadism|Sex Mania|Theophobia|Toxicophobia|Violent Temper|Xenophobia|Yaelaphobia"
list: deity-l
|" Agrik|Halea|Ilvir|Larani|Morgath|Naveh|Peoni|Sarajin|Save-K'nor|Siem|None"
: harn-attr-name ( a -- c-addr u ) >r attr-l list r> |field ;
: harn-species-name ( i -- c-addr u ) >r s" Human|Sindarin|Khuzdul|Other" r> |field ;
: harn-sex-name ( i -- c-addr u ) >r s" Female|Male" r> |field ;
: harn-month-name ( m -- c-addr u ) >r month-l list r> 1- |field ;
: harn-sign-name ( i -- c-addr u ) >r sign-l list r> |field ;
: harn-culture-name ( i -- c-addr u ) >r s" Tribal|Viking|Feudal|Imperial|Khuzan|Sindarin" r> |field ;
: harn-class-name ( i -- c-addr u ) >r s" Slave|Serf|Unguilded|Guilded|Noble" r> |field ;
: harn-parent-name ( i -- c-addr u ) >r s" Offspring|Fostered|Adopted|Bastard|Orphan" r> |field ;
: harn-estrangement-name ( i -- c-addr u ) >r s" Outcast|Unpopular|Average|Popular|Favorite" r> |field ;
: harn-clanhead-name ( i -- c-addr u ) >r s" Distant Relation|Aunt or Uncle|Cousin|Father or Mother" r> |field ;
: harn-frame-name ( i -- c-addr u ) >r s" Scant|Light|Medium|Heavy|Massive" r> |field ;
: harn-complexion-name ( i -- c-addr u ) >r s" Fair|Medium|Dark" r> |field ;
: harn-hair-name ( i -- c-addr u ) >r s" Brown|Black|Red|Silver|Blond" r> |field ;
: harn-eye-name ( i -- c-addr u ) >r s" Hazel|Gray|Violet|Green|Blue" r> |field ;
: harn-medical-name ( m -- c-addr u ) >r medical-l list r> |field ;
: harn-psyche-name ( p -- c-addr u ) >r psyche-l list r> |field ;
: harn-sexuality-name ( i -- c-addr u ) >r s" Heterosexual|Bi-Sexual|Homosexual" r> |field ;
: harn-deity-name ( i -- c-addr u ) >r deity-l list r> |field ;
create comeliness-t 5 , 8 , 12 , 15 , 99 ,
: harn-comeliness-name ( n -- c-addr u )
  comeliness-t 5 htable >r s" Ugly|Plain|Average|Attractive|Handsome" r> |field ;

\ A sunsign's signs (Character 3): its own, then the other of a cusp (the one before for its first days, after for
\ its last; its own again for the sign alone)
: harn-sunsign-signs ( ss -- s1 s2 )
  3 /mod swap  dup 1 = if drop dup exit then  0= if dup 11 + 12 mod else dup 1+ 12 mod then ;
create sunsign-buf 32 allot   variable sunsign-n
: sunsign+ ( c-addr u -- ) tuck sunsign-buf sunsign-n @ + swap move sunsign-n +! ;
: harn-sunsign-name ( ss -- c-addr u )
  0 sunsign-n !  harn-sunsign-signs 2dup = if drop harn-sign-name exit then
  swap harn-sign-name sunsign+ s" -" sunsign+ harn-sign-name sunsign+ s"  Cusp" sunsign+ sunsign-buf sunsign-n @ ;

\ ---- Birth (Character 2-3)
\ Each sign's first day, as a day of the year (0 the 1st of Nuzyael): Ulandus from the 4th of Nuzyael ... Lado from
\ the 2nd of Morgat to the 3rd of Nuzyael; a sign's first two days and last two its cusps
create sign-starts 3 , 33 , 62 , 93 , 124 , 156 , 185 , 214 , 243 , 272 , 302 , 331 ,
: s-start ( s -- day ) cells sign-starts + @ ;
variable ss-len
: harn-sunsign-of ( m d -- ss )
  swap 1- 30 * + 1-
  11 begin dup if 2dup s-start < else false then while 1- repeat
  dup 0= if over 3 < if drop 11 then then
  dup s-start >r  dup 11 < if dup 1+ s-start else 363 then r@ - ss-len !
  swap 360 + r> - 360 mod
  dup 2 < if drop 0 else ss-len @ 2 - < if 1 else 2 then then  swap 3 * + ;

\ Each culture's upper bounds for slave, serf, unguilded, guilded (noble the rest).  (The Viking column has unguilded
\ 81-93 and guilded 86-98: here unguilded 81-85.  Feudal slaves are Rethem's, serfs elsewhere: the table as it is)
create class-t  10 , 10 , 99 , 99 , 100 ,   15 , 80 , 85 , 98 , 100 ,   15 , 70 , 93 , 98 , 100 ,
                25 , 25 , 90 , 98 , 100 ,   0 , 0 , 15 , 98 , 100 ,     0 , 0 , 70 , 99 , 100 ,
create species-t 97 , 98 , 99 , 100 ,
create sex-t 48 , 45 , 75 , 48 ,             \ (The highest roll of a male, by species)
create culture-t 2 c, 5 c, 4 c, 2 c,          \ (Feudal, Sindarin, Khuzan, Feudal)
create sibs-t 25 , 50 , 70 , 85 , 95 , 100 ,
create parent-t 50 , 70 , 75 , 90 , 100 ,
create estrange-t 10 , 40 , 60 , 95 , 100 ,
create clanhead-t 50 , 75 , 85 , 100 ,

\ ---- Appearance (Character 6)
create height-t 52 c, 50 c, 40 c, 52 c,   54 c, 51 c, 40 c, 54 c,     \ (Female, then male: and 4d6)
create frame-t 5 , 8 , 12 , 15 , 99 ,
\ The Height/Weight table: 75 pounds at 40", 2 more an inch to 51", 3 from 52", 4 from 64", 3 at 70", 5 from 71"
\ (to 89": 255); then the frame's -20% ... +20%, rounded
: harn-weight ( ht fr -- wt )
  swap 40 max 89 min
  dup 52 < if 40 - 2* 75 + else dup 64 < if 51 - 3 * 97 + else dup 70 < if 63 - 4 * 133 + else 70 - 5 * 160 +
  then then then  swap 8 + * 5 + 10 / ;
\ Complexion, hair and eyes: each species' upper bounds (Human, Sindarin, Khuzdul, Other as Human)
create complexion-t 27 , 74 , 100 ,   97 , 99 , 100 ,   90 , 100 , 100 ,   27 , 74 , 100 ,
create hair-t 40 , 55 , 65 , 70 , 100 ,   25 , 48 , 50 , 60 , 100 ,
               40 , 45 , 70 , 90 , 100 ,   40 , 55 , 65 , 70 , 100 ,
create eyes-t 40 , 55 , 56 , 70 , 100 ,   15 , 65 , 75 , 80 , 100 ,
               50 , 69 , 70 , 85 , 100 ,   40 , 55 , 56 , 70 , 100 ,
\ Hair's and eyes' roll: humans +25 for a fair complexion, -25 for a dark one
: colour-roll ( -- r )
  d100  harn-sp harn@ dup 0= swap 3 = or if
    harn-complexion c@ case 0 of 25 + endof 2 of 25 - endof endcase
  then  1 max 100 min ;

\ ---- The attributes' modifiers (Character 6-10): species and sex, frame (Agility: scant +2 ... massive -2), weight
\ (Strength: 56-85 -4 ... 146-155 0 ... 216-245 +4, the table's ends past it); each species' by attribute
create sin-m 0 c, 0 c, 0 c, 0 c, 0 c, -2 c, 0 c, 0 c, 2 c, 1 c, 1 c, 2 c,
              2 c, 2 c, 2 c, 3 c, 2 c, 0 c, 4 c, 0 c, 3 c, 0 c, 0 c,
create khu-m 0 c, 0 c, 0 c, 0 c, 0 c, 3 c, 0 c, 0 c, 0 c, 4 c, 4 c, 1 c,
              0 c, 1 c, 2 c, 2 c, 0 c, 0 c, -2 c, 3 c, 0 c, 0 c, 0 c,
: sbyte ( c -- n ) dup 127 > if 256 - then ;
: harn-species-mod ( a -- n )
  harn-sp harn@ case
    0 of harn-sx harn@ if drop 0 else dup harn-fr = if drop -3 else harn-aur = 2 and then then endof
    1 of sin-m + c@ sbyte endof
    2 of khu-m + c@ sbyte endof
    nip 0 swap
  endcase ;
create weight-t 85 , 110 , 130 , 145 , 155 , 170 , 190 , 215 , 9999 ,
\ An attribute rolled: 3d6 and its modifiers, at least 1
: harn-rand-attr ( a -- n )
  3 6 dice over harn-species-mod +
  over harn-agl = if 2 harn-fr harn@ - + then
  swap harn-str = if harn-wt harn@ weight-t 9 htable 4 - + then  1 max ;

\ ---- Medical traits (Character 9): each trait's upper bound (1, Addiction ... 23, Sterile), a male's roll, a female's
create med-m 8 , 9 , 14 , 17 , 20 , 24 , 25 , 27 , 28 , 30 , 31 , 32 ,
             33 , 35 , 39 , 41 , 42 , 44 , 47 , 57 , 60 , 63 , 65 ,
create med-f 5 , 6 , 10 , 14 , 15 , 19 , 20 , 22 , 23 , 25 , 26 , 27 ,
             29 , 30 , 40 , 42 , 43 , 45 , 48 , 58 , 61 , 63 , 65 ,
variable int-bonus                            \ (Epilepsy's: Intelligence isn't rolled yet)
: lower ( a n -- ) over harn@ swap - 1 max swap harn! ;
: more ( a n -- ) over harn@ + swap harn! ;
\ A trait's effects (dwarfism's and gigantism's on height aren't made: the height's taken)
: lower? ( m m2 a -- m ) \ (A trait m2's: its attribute a less 1d3)
  >r over = if r> 3 die lower else r> drop then ;
: medical-effect ( m -- )
  dup 4 = if drop harn-dex 3 die more exit then
  dup 8 = if drop 3 die int-bonus ! exit then
  dup 14 = if drop harn-agl dup harn@ 2/ lower exit then
  dup 15 = if drop harn-dex 2 die more exit then
  dup 16 = if drop harn-cml 2 6 dice lower exit then
  dup 19 = if drop harn-wt harn@ 51 random 150 + 100 */ harn-wt harn!  harn-agl 3 die lower exit then
  5 harn-dex lower?  6 harn-cml lower?  13 harn-cml lower?  21 harn-cml lower?  22 harn-cml lower?
  9 harn-hrg lower?  10 harn-eye lower?  18 harn-eye lower?  drop ;
\ A roll on the table: a trait (with its effects; each once, four at most), multiple (two rolls more), or none
: medical ( depth -- )
  d100  dup 70 > if 2drop exit then
  dup 65 > if drop dup 3 < if 1+ dup recurse recurse else drop then exit then
  nip  harn-sx harn@ if med-m else med-f then 23 htable 1+
  4 0 do  i harn-med c@ dup 0= if drop dup i harn-med c! medical-effect unloop exit then
    over = if drop unloop exit then  loop drop ;

\ ---- Mental disorders and sexuality (Character 11: optional rules, not made): a disorder (1, Acrophobia ... 38,
\ Yaelaphobia) or more, each with its severity (0 mild, 1 moderate, 2 severe); a sexuality (0 heterosexual, 1
\ bisexual, 2 homosexual)
create psyche-t 3 , 5 , 6 , 8 , 9 , 11 , 14 , 15 , 17 , 19 , 22 , 24 , 25 , 26 , 28 , 31 , 32 , 35 , 36 , 37 , 38 , 40 ,
  43 , 45 , 46 , 47 , 48 , 49 , 51 , 54 , 56 , 57 , 59 , 60 , 61 , 63 , 64 , 65 ,
create severity-t 60 , 90 , 100 ,
: harn-rand-psyche ( depth -- )
  d100  dup 70 > if 2drop exit then
  dup 65 > if drop dup 3 < if 1+ dup recurse recurse else drop then exit then
  nip psyche-t 38 htable 1+  d100 severity-t 3 htable
  3 0 do i harn-psy c@ 0= if i harn-psysev c! i harn-psy c! unloop exit then loop 2drop ;
create sexuality-t 90 , 96 , 100 ,
: harn-rand-sexuality ( -- s ) d100 sexuality-t 3 htable ;

\ ---- Skills (Skills 3-4: the Skill Data table, its order): a line a skill, 41 bytes: its key (harn.dl's, the file's),
\ its name, its three attributes (S STR, T STA, D DEX, A AGL, E EYE, H HRG, M SML, V VOI, I INT, U AUR, W WIL, C CML),
\ its sunsign bonuses (each a sign, a Ulandus ... l Lado, and its bonus), its OML's multiple of SB (L a native tongue,
\ 50-80 + SB by social class; S a native script, 70 + SB), 1 if it's automatic, its group (0 physical, 1
\ communication, 2 religion, 3 combat, 4 craft: physical and combat skills take the physical penalty, the rest the
\ universal), and a craft's months of teaching (0: opened on its first use).  hylang's harn.hl has the same table.
91 constant harn-nskills
: sk" ( "line<quote>" -- ) [char] " parse  here swap dup allot move ;
create harn-skills
sk" acr  Acrobatics         SAA f2g1     2000"
sk" cli  Climbing           SDA a2b2     4100"
sk" cnd  Condition          STW a1l1     5100"
sk" dan  Dancing            DAA h2g1i1   2000"
sk" jmp  Jumping            SAA f2g2     4100"
sk" leg  Legerdemain        DDW j2i2h2   1000"
sk" ski  Skiing             SDA k2j1l1   1000"
sk" stl  Stealth            AHW g2h2i2   3100"
sk" swi  Swimming           TDA j1k3l3   1000"
sk" thr  Throwing           SDE g2h1f1   4100"
sk" act  Acting             AVI h1i1     2010"
sk" awa  Awareness          EHM g2h2     4110"
sk" ntr  Intrigue           IUW i1h1j1   3110"
sk" lov  Lovecraft          CAV k1e1     3010"
sk" men  Mental Conflict    UWW          3010"
sk" mus  Musician           DHH k1e1     1010"
sk" ora  Oratory            CVI h1       2110"
sk" rhe  Rhetoric           VIW i1h1j1   3110"
sk" sng  Singing            HVV k1       3110"
sk" lan  Language           VIW i1       L110"
sk" scr  Script             DEI h1i1     S011"
sk" ragr Ritual: Agrik      VIS f2e1d1   1020"
sk" rhal Ritual: Halea      VIC h2g1k1   1020"
sk" rilv Ritual: Ilvir      VIU j2i1a1   1020"
sk" rlar Ritual: Larani     VIW e2d1c1   1020"
sk" rmor Ritual: Morgath    VIU l2d1k1   1020"
sk" rnav Ritual: Naveh      VIW k2j1h1   1020"
sk" rpeo Ritual: Peoni      VID b2e1a1   1020"
sk" rsar Ritual: Sarajin    VIS c2b1l1   1020"
sk" rsav Ritual: Save-K'nor VII i2j1h1   1020"
sk" rsie Ritual: Siem       VIU g2c1a1   1020"
sk" ini  Initiative         AWW          4130"
sk" una  Unarmed            SDA k2l2a2   4130"
sk" rid  Riding             DAW a1b1     1030"
sk" axe  Axe                SSD d1c1e1   3030"
sk" blo  Blowgun            TDE g2h1f1   4030"
sk" bow  Bow                SDE g1h1f1   2030"
sk" clu  Club               SSD a1b1     4030"
sk" dag  Dagger             DDE e2f2     3030"
sk" fla  Flail              DDD g1h1f1   1030"
sk" net  Net                DDE k1j1l1   1030"
sk" pol  Polearm            SSD e1b1     2030"
sk" shd  Shield             SDD a1l1k1   3030"
sk" slg  Sling              DDE g1h1f1   1030"
sk" spr  Spear              SSD b1c1a1   3030"
sk" swd  Sword              SDD e3d1f1   3030"
sk" whp  Whip               DDE g1f1     1030"
sk" dge  Dodge              AAA          5130"
sk" agr  Agriculture        STW a2b2     2041"
sk" alc  Alchemy            MIU j3i2k2   1046"
sk" ani  Animalcraft        AVW a1b1     1041"
sk" ast  Astrology          EIU h1       1043"
sk" bre  Brewing            DMM j3i2k2   2041"
sk" cer  Ceramics           DDE a2b2     2042"
sk" coo  Cookery            DMM j1       3040"
sk" dra  Drawing            DEE j1i1     2041"
sk" emb  Embalming          DEM j1a1     1042"
sk" eng  Engineering        DII a2b2c1   1043"
sk" fis  Fishing            DEW k2l2     3040"
sk" fle  Fletching          DDE g2h1f1   1041"
sk" fol  Folklore           VII i2       3040"
sk" for  Foraging           DMI a2b2     3040"
sk" gla  Glassworking       DEW c2       1043"
sk" hrl  Heraldry           DEW j1i1     1041"
sk" hrb  Herblore           EMI a3b2     1041"
sk" hid  Hidework           DMW a1b1     2041"
sk" hun  Hunting            AMI a2b2     2040"
sk" ink  Inkcraft           EMI j2i1     1041"
sk" jew  Jewelcraft         DEW c3h1b1   1042"
sk" law  Law                VIW h1i1     1042"
sk" lck  Lockcraft          DEW c1       1042"
sk" lor  Lore               EII i2       2041"
sk" msn  Masonry            SDI a2b2     1041"
sk" mat  Mathematics        IIW i3h1j1   1043"
sk" met  Metalcraft         SDW c3d1e1   1042"
sk" mil  Milling            SDM a1       2041"
sk" min  Mining             SEI a2b2c1   1041"
sk" prf  Perfumery          MMI g1j1h1   1041"
sk" phy  Physician          DEI k2j1i1   1040"
sk" pil  Piloting           DEI l3k1     1046"
sk" run  Runecraft          IUU i2j1     1046"
sk" sea  Seamanship         SDA l3k1j1   2041"
sk" shw  Shipwright         SDI l3k1     1042"
sk" sur  Survival           SDI a2b1     3041"
sk" trt  Tarotry            IUW h2i2j1g1 1044"
sk" tex  Textilecraft       DDE a1b1     2041"
sk" tim  Timbercraft        SDA a3b1     2041"
sk" trk  Tracking           EMW a3b3     2041"
sk" wpn  Weaponcraft        SDW c3d1e1   1044"
sk" wea  Weatherlore        IEM g1h1k1l1 3041"
sk" woo  Woodcraft          DDW a2b1l1   2041"

: sk-row ( sk -- addr ) 41 * harn-skills + ;
: harn-skill-key ( sk -- c-addr u ) sk-row 5 -trailing ;
: harn-skill-name ( sk -- c-addr u ) sk-row 5 + 19 -trailing ;
: harn-skill-group ( sk -- g ) sk-row 39 + c@ [char] 0 - ;
: harn-skill-auto ( sk -- flag ) sk-row 38 + c@ [char] 1 = ;
\ A skill by its key (-1: none)
: harn-skill-of ( c-addr u -- sk )
  harn-nskills 0 do 2dup i harn-skill-key compare 0= if 2drop i unloop exit then loop 2drop -1 ;

\ ---- A character's skills
\ The average of three attributes, rounded (half up); Endurance: Strength's, Stamina's and Will's
: harn-avg ( a b c -- n ) + + 2* 3 + 6 / ;
: harn-end ( -- n ) harn-str harn@ harn-sta harn@ harn-wil harn@ harn-avg ;
\ A skill's attribute i (0-2), by its letter
create sk-codes 9 c, 10 c, 11 c, 12 c, 13 c, 14 c, 15 c, 16 c, 17 c, 18 c, 19 c, 8 c,
: sk-att ( sk i -- n )
  swap sk-row 24 + + c@  s" STDAEHMVIUWC" 0 do 2dup i + c@ = if 2drop i leave then loop  sk-codes + c@ harn@ ;
\ The best bonus of a skill's signs that the character's sunsign has (a cusp: either of its two)
variable sign1   variable sign2
: harn-ssb ( sk -- b )
  harn-ss harn@ harn-sunsign-signs [char] a + sign2 ! [char] a + sign1 !
  sk-row 28 +  0 swap  4 0 do
    dup c@ dup bl = if drop leave then
    dup sign1 @ = swap sign2 @ = or if dup 1+ c@ [char] 0 - rot max swap then  2 +
  loop drop ;
\ SB: the three attributes' average and the sunsign's bonus (Condition's SB is Endurance: Skills 9)
: harn-sb ( sk -- sb )
  dup 2 = if drop harn-end exit then
  dup 0 sk-att over 1 sk-att 2 pick 2 sk-att harn-avg  swap harn-ssb + ;
\ OML: SB times its multiple; a native script 70 + SB, a native tongue 50-80 + SB by social class (Skills 11)
: harn-oml ( sk -- oml )
  dup sk-row 37 + c@ swap harn-sb swap
  case
    [char] S of 70 + endof
    [char] L of harn-social c@ dup 2 < if drop 50 else 4 = if 70 else 60 then then + endof
    dup [char] 0 - rot * swap
  endcase ;
: harn-ml@ ( sk -- ml ) harn-ml c@ ;
: harn-ml! ( ml sk -- ) swap 255 min swap harn-ml c! ;
: harn-open ( sk -- ) dup harn-ml@ if drop else dup harn-oml swap harn-ml! then ;

\ ---- Penalties (Skills 6), move (Combat 6), size (Character 6)
\ UP: the fatigue levels (and the injuries' levels: not kept here); PP: UP and EP, the load over endurance, rounded
: harn-up ( -- n ) harn-fl c@ ;
: harn-ep ( -- n ) harn-load @ 2* harn-end + harn-end 2* / ;
: harn-pp ( -- n ) harn-up harn-ep + ;
: harn-move ( -- n ) harn-agl harn@ harn-pp - 1 max ;
create size-t 65 , 75 , 90 , 110 , 135 , 165 , 200 , 240 , 285 , 9999 ,
: harn-size ( -- n ) harn-wt harn@ size-t 10 htable 1+ ;

\ ---- Tests (Skills 5, 21-22) and development (Skills 6)
\ d100 at or under the EML (kept from 5 to 95): 0 CF, 1 MF, 2 MS, 3 CS (a roll ending in 0 or 5 critical)
: harn-test ( eml -- level )
  5 max 95 min  d100 tuck < 0= 2 and  swap 5 mod 0= if 1+ else 1- abs then ;
\ (The roll at or under: 2, critical 3; over: 1, critical 0)
: harn-eml ( sk -- eml )
  dup harn-ml@ swap harn-skill-group dup 0= swap 3 = or if harn-pp else harn-up then 5 * - ;
: harn-test-skill ( sk mod -- level ) swap harn-eml + harn-test ;
\ d100 and SB over ML raise it by 1 (it can't pass 100 + SB): 0 improved, 1 not improved, 2 maxed
: harn-develop ( sk -- r )
  dup harn-ml@ over harn-sb  2dup 100 + < 0= if 2drop drop 2 exit then
  d100 + < if dup harn-ml@ 1+ swap harn-ml! 0 else drop 1 then ;

\ ---- A new character (Character 2-12), in harn_create's order (harngen.c's): birth, culture and class, family,
\ appearance, the physical attributes, the medical traits, the personality, piety; then the automatic skills opened
: harn-create ( -- )
  harn-c /harn erase  0 int-bonus !
  d100 species-t 4 htable harn-sp harn!
  d100 harn-sp harn@ cells sex-t + @ > 0= 1 and harn-sx harn!
  12 die 30 die  over 8 lshift over or harn-bd harn!  harn-sunsign-of harn-ss harn!
  harn-sp harn@ culture-t + c@ harn-culture c!
  d100 harn-culture c@ 5 * cells class-t + 5 htable harn-social c!
  d100 harn-sp harn@ 1 3 within if 20 - then 1 max sibs-t 6 htable 1+ harn-sibrank c!
  harn-sibrank c@ 6 die + 1- harn-famsize c!
  d100 parent-t 5 htable harn-parent c!
  d100 harn-sibrank c@ 1 = 5 and + 100 min estrange-t 5 htable harn-estrange c!
  d100 clanhead-t 4 htable harn-clanhead c!
  harn-sx harn@ 4 * harn-sp harn@ + height-t + c@ 4 6 dice + harn-ht harn!
  3 6 dice harn-fr harn-species-mod + frame-t 5 htable harn-fr harn!
  harn-ht harn@ harn-fr harn@ harn-weight dup harn-owt harn! harn-wt harn!
  harn-cml harn-rand-attr harn-cml harn!
  d100 harn-sp harn@ 3 * cells complexion-t + 3 htable harn-complexion c!
  colour-roll harn-sp harn@ 5 * cells hair-t + 5 htable harn-hair c!
  colour-roll harn-sp harn@ 5 * cells eyes-t + 5 htable harn-eyes c!
  harn-voi 1+ harn-str do i harn-rand-attr i harn! loop
  0 medical
  harn-mor 1+ harn-int do i harn-rand-attr i harn! loop
  harn-int int-bonus @ more
  10 harn-dty harn!  5 6 dice harn-pty harn!
  harn-nskills 0 do i harn-skill-auto if i harn-open then loop ;

\ ---- Files (harnfile.c's): harn 1; att KEY VALUE (each attribute, in order); fam culture social sibrank famsize parent
\ estrange clanhead; look complexion hair eyes; med (the traits); psy DISORDER SEVERITY (a line each); sexuality N; ml
\ KEY ML (each open skill); vol fl bl dp state shockhr; then the program's own lines
create harn-line 100 allot   variable harn-n
: line{ ( -- ) 0 harn-n ! ;
: line+ ( c-addr u -- ) tuck harn-line harn-n @ + swap move harn-n +! ;
: line+n ( u -- ) s"  " line+ 0 <# #s #> line+ ;
: line+bytes ( addr n -- ) 0 ?do dup i + c@ line+n loop drop ;
variable harn-fd
: line} ( -- ior ) harn-line harn-n @ harn-fd @ write-line ;
: harn-write-line ( c-addr u fileid -- ior ) write-line ;
: harn-att-key ( a -- c-addr u )                 \ Its key in a file: harn.dl's
  >r s" sp|sx|bd|ss|ht|fr|owt|wt|cml|str|sta|dex|agl|eye|hrg|sml|voi|int|aur|wil|mor|dty|pty" r> |field ;
: harn-write ( fileid -- ior )
  harn-fd !  s" harn 1" harn-fd @ write-line
  harn-nattrs 0 do line{ s" att " line+ i harn-att-key line+ i harn@ line+n line} or loop
  line{ s" fam" line+ harn-culture 7 line+bytes line} or
  line{ s" look" line+ harn-complexion 3 line+bytes line} or
  line{ s" med" line+ 4 0 do i harn-med c@ ?dup if line+n else leave then loop line} or
  3 0 do i harn-psy c@ ?dup if line{ s" psy" line+ line+n i harn-psysev c@ line+n line} or then loop
  line{ s" sexuality" line+ harn-sexuality 1 line+bytes line} or
  100 0 do i harn-ml@ ?dup if
    line{ s" ml " line+ i harn-nskills < if i harn-skill-key line+ else i 0 <# #s #> line+ then line+n line} or
  then loop
  line{ s" vol" line+ harn-fl 5 line+bytes line} or ;


\ ---- Reading one back: each line by its first word; any other line harn-other's
defer harn-other ( c-addr u -- )   ' 2drop is harn-other
create harn-in 102 allot   create harn-rest 2 cells allot
: next-word ( -- c-addr u )                          \ The line's next word (spaces before it skipped)
  harn-rest 2@ begin dup if over c@ bl = else false then while 1 /string repeat
  over swap begin dup if over c@ bl <> else false then while 1 /string repeat  ( a a' u' )
  2dup harn-rest 2!  drop over - ;
: next-num ( -- n ) next-word 0 0 2swap >number 2drop drop ;
: next-bytes ( addr n -- ) 0 ?do next-num over i + c! loop drop ;
: word= ( c-addr u c-addr2 u2 -- flag ) compare 0= ;
: harn-read-line ( c-addr u -- )
  2dup harn-rest 2!  next-word
  2dup s" att" word= if 2drop 2drop next-word
    harn-nattrs 0 do 2dup i harn-att-key word= if 2drop next-num i harn! unloop exit then loop 2drop exit then
  2dup s" fam" word= if 2drop 2drop harn-culture 7 next-bytes exit then
  2dup s" look" word= if 2drop 2drop harn-complexion 3 next-bytes exit then
  2dup s" med" word= if 2drop 2drop 0 harn-med 4 erase
    4 0 do next-num ?dup if i harn-med c! else leave then loop exit then
  2dup s" psy" word= if 2drop 2drop
    3 0 do i harn-psy c@ 0= if next-num i harn-psy c! next-num i harn-psysev c! leave then loop exit then
  2dup s" sexuality" word= if 2drop 2drop harn-sexuality 1 next-bytes exit then
  2dup s" ml" word= if 2drop 2drop next-word 2dup s" tar" word= if 2drop s" trt" then   \ (C's key for Tarotry)
    over c@ [char] 0 [char] 9 1+ within if 0 0 2swap >number 2drop drop else harn-skill-of then
    dup 0< if drop else next-num swap harn-ml! then exit then
  2dup s" vol" word= if 2drop 2drop harn-fl 5 next-bytes exit then
  2dup s" harn" word= if 2drop 2drop exit then
  2drop harn-other ;
: harn-read ( fileid -- ior )
  >r harn-c /harn erase
  begin harn-in 100 r@ read-line ?dup if nip nip r> drop exit then
  while harn-in swap harn-read-line repeat
  drop r> drop 0 ;
