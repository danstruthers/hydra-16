\ video.fs - the Vera X's screen, by its files (#v, /dev/vid): lib video.  hylang's video library's words, its order.
\ A bitmap under the console's text (layer 0: ctl's bitmap), drawn on by vid itself (/dev/vid/draw), so the console
\ stays on the screen over it:
\   bitmap ( width depth -- )   320 or 640 across, 1, 2, 4 or 8 bits a pixel (640: 1 or 2); bitmap-off ( -- )
\   pen ( colour -- )           the colour drawn in (vid's, one for all)
\   plot ( x y -- )  line ( x0 y0 x1 y1 -- )  box ( x0 y0 x1 y1 -- )  bar ( x0 y0 x1 y1 -- ) (filled)
\   circle ( x y r -- )  disc ( x y r -- ) (filled)  clear ( -- )
\   text ( x y c-addr u -- )    in the console's font, 8 x 8, its dots in the pen's colour (40 characters at most)
\ Turtle graphics on it, Logo's: cs (cleared, the turtle home), home (the middle, heading up, its pen down), fd ( n
\ -- ), bk ( n -- ), rt ( degrees -- ), lt ( degrees -- ), pu, pd, heading ( -- degrees ), seth ( degrees -- ).
\ The chip's parts: vpoke ( addr bank c -- ), vpeek ( addr bank -- c ) (VRAM: a 17-bit address, its 16 bits and bit
\ 16), vram! ( addr bank c-addr u -- ), vram@ ( addr bank c-addr u -- ), palette! ( index rgb -- ) ($RGB, 4 bits
\ each), sprite! ( n c-addr -- ) (sprite n's 8 bytes), sprite-at ( n x y -- ), sprite-off ( n -- ), vsync ( -- ) (the
\ next frame), border ( colour -- ).  The mouse: mouse ( -- x y buttons ) (as it is now), mouse-wait ( -- x y buttons )
\ (its next change; the buttons 1 left, 2 middle, 4 right).  A failure THROWs (a bad number, or no bitmap: E_INVAL;
\ the chip claimed: E_BUSY).  Only the words forth starts with (File Access's).

create vid-buf 64 allot   variable vid-n         \ A command being made
create vid-rec 49 allot                           \ A /mouse record (Plan 9's: m, then 4 fields of 11 and a space)
create vid-4 4 allot                              \ Bytes for a file
variable vid-dfd   0 vid-dfd !                    \ /dev/vid/draw, open once it's wanted ...
variable vid-vfd   0 vid-vfd !                    \   /dev/vid/vram ...
variable vid-ffd   0 vid-ffd !                    \   /dev/vid/frame ...
variable vid-mfd   0 vid-mfd !                    \   and /dev/vid/mouse, for mouse-wait
variable vid-w   320 vid-w !   variable vid-h   240 vid-h !   \ The bitmap's size (bitmap's)

: vid{ ( -- ) 0 vid-n ! ;
: vid+ ( c-addr u -- ) tuck vid-buf vid-n @ + swap move vid-n +! ;
: vid+n ( n -- ) bl vid-buf vid-n @ + c! 1 vid-n +!            \ A space, then the number (signed)
  base @ >r decimal dup abs 0 <# #s rot sign #> vid+ r> base ! ;
: vid} ( -- c-addr u ) vid-buf vid-n @ ;
: vid-write ( c-addr u c-addr2 u2 -- ) w/o open-file throw >r r@ write-file r> close-file throw throw ;
: vid-ctl ( c-addr u -- ) s" /dev/vid/ctl" vid-write ;
: vid-put ( c-addr u ud c-addr2 u2 -- )           \ u bytes into the file, at offset ud
  r/w bin open-file throw >r r@ reposition-file throw r@ write-file r> close-file throw throw ;
: vid-draw ( c-addr u -- )                        \ A command to /dev/vid/draw
  vid-dfd @ 0= if s" /dev/vid/draw" w/o open-file throw vid-dfd ! then vid-dfd @ write-file throw ;
: vid2 ( a b c-addr u -- ) vid{ vid+ swap vid+n vid+n vid} vid-draw ;
: vid3 ( a b c c-addr u -- ) vid{ vid+ rot vid+n swap vid+n vid+n vid} vid-draw ;
: vid4 ( a b c d c-addr u -- ) vid{ vid+ 2swap swap vid+n vid+n swap vid+n vid+n vid} vid-draw ;

\ The bitmap
: bitmap ( width depth -- )
  over dup vid-w ! 320 = if 240 else 480 then vid-h !  vid{ s" bitmap" vid+ swap vid+n vid+n vid} vid-ctl ;
: bitmap-off ( -- ) s" bitmap off" vid-ctl ;
: border ( colour -- ) vid{ s" border" vid+ vid+n vid} vid-ctl ;

\ Drawing on it
: pen ( colour -- ) vid{ s" pen" vid+ vid+n vid} vid-draw ;
: plot ( x y -- ) s" plot" vid2 ;
: line ( x0 y0 x1 y1 -- ) s" line" vid4 ;
: box ( x0 y0 x1 y1 -- ) s" box" vid4 ;
: bar ( x0 y0 x1 y1 -- ) s" bar" vid4 ;
: circle ( x y r -- ) s" circle" vid3 ;
: disc ( x y r -- ) s" disc" vid3 ;
: clear ( -- ) s" clear" vid-draw ;
: text ( x y c-addr u -- )
  2>r vid{ s" text" vid+ swap vid+n vid+n  bl vid-buf vid-n @ + c! 1 vid-n +!  2r> 40 min vid+ vid} vid-draw ;

\ The turtle: its place in 16ths of a pixel, its heading in degrees (0 up, clockwise), its pen
create vid-sin                                    \ sin of 0-90 degrees, x 10000
  0 , 175 , 349 , 523 , 698 , 872 , 1045 , 1219 , 1392 , 1564 , 1736 , 1908 , 2079 ,
  2250 , 2419 , 2588 , 2756 , 2924 , 3090 , 3256 , 3420 , 3584 , 3746 , 3907 , 4067 , 4226 ,
  4384 , 4540 , 4695 , 4848 , 5000 , 5150 , 5299 , 5446 , 5592 , 5736 , 5878 , 6018 , 6157 ,
  6293 , 6428 , 6561 , 6691 , 6820 , 6947 , 7071 , 7193 , 7314 , 7431 , 7547 , 7660 , 7771 ,
  7880 , 7986 , 8090 , 8192 , 8290 , 8387 , 8480 , 8572 , 8660 , 8746 , 8829 , 8910 , 8988 ,
  9063 , 9135 , 9205 , 9272 , 9336 , 9397 , 9455 , 9511 , 9563 , 9613 , 9659 , 9703 , 9744 ,
  9781 , 9816 , 9848 , 9877 , 9903 , 9925 , 9945 , 9962 , 9976 , 9986 , 9994 , 9998 , 10000 ,
variable tx   variable ty   variable th   variable tpen
: vid-deg ( n -- 0..359 ) 360 mod dup 0< if 360 + then ;
: vid-sin@ ( degrees -- n ) vid-deg                \ x 10000
  dup 90 > if dup 180 > if 180 - recurse negate exit then 180 swap - then cells vid-sin + @ ;
: vid-cos@ ( degrees -- n ) 90 + vid-sin@ ;
: home ( -- ) vid-w @ 8 * tx !  vid-h @ 8 * ty !  0 th !  true tpen ! ;
: cs ( -- ) clear home ;
: pu ( -- ) false tpen ! ;
: pd ( -- ) true tpen ! ;
: rt ( degrees -- ) th +! ;
: lt ( degrees -- ) negate rt ;
: heading ( -- degrees ) th @ vid-deg ;
: seth ( degrees -- ) th ! ;
: fd ( n -- )                                     \ Forward n pixels, a line drawn with the pen down
  tx @ 16 / ty @ 16 / 2>r
  dup heading vid-sin@ 625 */ tx +!  heading vid-cos@ 625 */ negate ty +!
  tpen @ if 2r> tx @ 16 / ty @ 16 / line else 2r> 2drop then ;
: bk ( n -- ) negate fd ;
home

\ The chip's parts
: vid-vram ( addr bank -- fid )                   \ /dev/vid/vram, at the address
  vid-vfd @ 0= if s" /dev/vid/vram" r/w bin open-file throw vid-vfd ! then
  1 and vid-vfd @ reposition-file throw vid-vfd @ ;
: vpoke ( addr bank c -- ) vid-4 c! vid-vram >r vid-4 1 r> write-file throw ;
: vpeek ( addr bank -- c ) vid-vram >r vid-4 1 r> read-file throw drop vid-4 c@ ;
: vram! ( addr bank c-addr u -- ) 2swap vid-vram write-file throw ;
: vram@ ( addr bank c-addr u -- ) 2swap vid-vram read-file throw drop ;
: palette! ( index rgb -- )                       \ The colour's entry: $GB, then $0R
  dup 255 and vid-4 c!  8 rshift 15 and vid-4 1+ c!  vid-4 2 rot 2* 0 s" /dev/vid/pal" vid-put ;
: sprite! ( n c-addr -- ) 8 rot 8 * 0 s" /dev/vid/sprites" vid-put ;
: sprite-at ( n x y -- ) vid-4 2 + ! vid-4 ! vid-4 4 rot 8 * 2 + 0 s" /dev/vid/sprites" vid-put ;
: sprite-off ( n -- ) 0 vid-4 c! vid-4 1 rot 8 * 6 + 0 s" /dev/vid/sprites" vid-put ;
: vsync ( -- )                                    \ The next frame (59.5 a second)
  vid-ffd @ 0= if s" /dev/vid/frame" r/o open-file throw vid-ffd ! then vid-buf 16 vid-ffd @ read-file throw drop ;

\ The mouse
: vid-num ( c-addr u -- n )                       \ A field: past its spaces, its number
  begin over c@ bl = over 0> and while 1- swap 1+ swap repeat 0. 2swap >number 2drop drop ;
: vid-fields ( -- x y buttons ) vid-rec 1+ 11 vid-num  vid-rec 13 + 11 vid-num  vid-rec 25 + 11 vid-num ;
: mouse ( -- x y buttons )                        \ As it is (an open's first read)
  s" /dev/vid/mouse" r/o open-file throw >r vid-rec 49 r@ read-file r> close-file throw throw drop vid-fields ;
: mouse-wait ( -- x y buttons )                   \ Its next change (a click's, each in turn)
  vid-mfd @ 0= if s" /dev/vid/mouse" r/o open-file throw vid-mfd !  vid-rec 49 vid-mfd @ read-file throw drop then
  vid-rec 49 vid-mfd @ read-file throw drop vid-fields ;
