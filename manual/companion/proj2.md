# Project 2. Turtle Pictures

> **THIS PROJECT:** HyForth, with a Vera X (the emulator has one). After Labs 16 and 17. Two sittings. You'll use
> `do` loops, words built from words, and the turtle of section 20.7.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

Imagine a turtle on the screen, with a pen tied to its tail. `fd` walks it forward, `rt` and `lt` turn it right
and left, by a number of degrees, and wherever it walks, it draws. Type this into `/sd/0/work/shapes.fs`, and run it
with `forth /sd/0/work/shapes.fs`.

<!-- listing samples/shapes.fs -->

```text
\ shapes.fs - turtle pictures (it needs a Vera X)
lib video

: polygon ( size sides -- )     \ A shape with that many sides, each that long
  360 over / swap               \ ( size turn sides )
  0 do over fd dup rt loop 2drop ;

: square   ( size -- ) 4 polygon ;
: triangle ( size -- ) 3 polygon ;

: flower ( -- )                 \ 36 squares, each turned a little, each a new colour
  36 0 do  i 15 mod 1+ pen  60 square  10 rt  loop ;

320 8 bitmap  cs
flower
```

<figure>
<img src="figures/forth-flower.png" alt="Thirty-six squares, each turned ten degrees from the last, in many colours, making a flower">
<figcaption>Thirty-six squares make a flower.</figcaption>
</figure>

To try words one at a time, start with `forth` alone, type `include /sd/0/work/shapes.fs`, and then things like
`cs 80 triangle` at its prompt. (`cs` clears the screen, and puts the turtle back in the middle.)

### Understand it first

1. ✎ To draw a square, the turtle turns 90 degrees at each corner. How far must it turn for a triangle? For a
   hexagon? What's the rule, and where is it in `polygon`?

<div class="write l3"></div>

2. ✎ Go through `polygon` a word at a time, for `60 4 polygon`, and write the stack after each word, up to the
   `do`.

<div class="write l3"></div>

3. ✎ What does `i 15 mod 1+ pen` do in `flower`? Why the `1+`?

<div class="write l2"></div>

### Make it yours

| ☐ | Draw |
|---|---|
| ☐ | A row of polygons, from a triangle to an octagon (`pu` and `pd` lift and lower the pen) |
| ☐ | A star: five lines, turning 144 degrees each time |
| ☐ | A spiral: a line a little longer each time round the loop, with the same turn |
| ☐ | A circle made by the turtle: many tiny steps, with a tiny turn |
| ☐ | A flower made of some other shape than a square |
| ☐ | Your initials |
| ☐ | A picture of your own, built from at least three words you've defined |

✎ Sketch your best picture, and write the words that drew it.

<div class="space s2"></div>

<div class="write l4"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | Each word has a stack diagram, and leaves the stack as its diagram says |
| ☐ | The picture is drawn by one word, built from smaller ones |
| ☐ | Someone else has changed a number in your file, and guessed what would happen |
| ☐ | It's saved in `/sd/0/work` |

> **CHALLENGE:** ★ A spiral where the turn is 90 degrees makes a square spiral. What about 91? Or 120, 121, 144,
> 170? Write `spiral` ( *turn* -- ), and explore. Which turns make stars, and why?
