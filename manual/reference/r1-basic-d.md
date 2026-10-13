## FOR ... NEXT

A statement block: a loop that counts a variable from one value to another.

#### Syntax

```text
FOR v = start TO limit [STEP step]
    [statements]
NEXT [v [, v] ...]
```

#### Arguments

*v*
: The loop's counter: a simple numeric variable. An array's element can't be one.

*start*, *limit*, *step*
: Numeric expressions. *step* is 1 if `STEP` is left out.

#### Description

1. *start* is assigned to *v*, and *limit* and *step* are evaluated and kept. They are evaluated this once only:
   a variable they were worked out from can change inside the loop without changing them.
2. The test: if *step* is zero or more and *v* is greater than *limit*, or *step* is negative and *v* is less than
   *limit*, the loop ends, and the program goes on after its `NEXT`. The test is made before the first pass too,
   so the statements may not run at all; *v* then keeps the value *start*.
3. Otherwise the statements run.
4. At `NEXT`, *step* is added to *v*, and the loop goes back to the test at step 2.

When the loop ends at the test, *v* holds the first value that failed it: after `FOR I = 1 TO 3`, `I` is 4.

The addition is exact. A loop with a fractional step, `STEP 0.1` or `STEP 1/3`, reaches its limit on the last
pass, where a binary floating-point BASIC may stop one short.

The statements may assign to *v*; `NEXT` adds *step* to whatever it then holds.

`EXIT FOR` leaves the innermost loop at once, with *v* as it is.

`NEXT` with no variable closes the innermost open loop. `NEXT v` does the same, and the compiler checks that *v*
is that loop's counter. `NEXT j, i` is `NEXT j : NEXT i`.

#### Errors

| `ERR` | Message | Cause |
|---|---|---|
| 26 | `FOR without NEXT` | A `FOR` whose `NEXT` isn't there. Found when the program is compiled: nothing has run |
| 1 | `NEXT without FOR` | A `NEXT` with no loop open, or naming a variable that isn't the innermost loop's counter |
| 80 | `block without its start or end` | The loop and another block overlap: a `NEXT` inside a one-line `IF`, say |
| 13 | `type mismatch` | *v* is a string variable, or *start*, *limit* or *step* a string |
| 2 | `syntax error` | *v* is not a simple variable |

#### Notes

With a *step* of 0 the counter never changes and the loop doesn't end by itself: only `EXIT FOR`, an error or
<kbd>CTRL</kbd>-<kbd>C</kbd> ends it.

At the prompt, a line is compiled alone, so a loop typed there must be whole on its line: `FOR I = 1 TO 3` by
itself is `FOR without NEXT`.

#### See also

`EXIT`, `DO`...`LOOP`, `WHILE`...`WEND`.

#### Example

```session
> FOR I = 1 TO 3 : PRINT I; : NEXT : PRINT "then"; I
 1  2  3 then 4
> FOR I = 5 TO 1 : PRINT I; : NEXT : PRINT "then"; I
then 5
> FOR X = 0 TO 1 STEP 1/3 : PRINT X; : NEXT : PRINT
 0  1/3  2/3  1
> N = 3 : FOR I = 1 TO N : N = 10 : PRINT I; : NEXT : PRINT
 1  2  3
> FOR I = 1 TO 2 : FOR J = 1 TO 2 : PRINT I; J; : NEXT J, I : PRINT
 1  1  1  2  2  1  2  2
> FOR I = 1 TO 2 : FOR J = 1 TO 2 : NEXT I
NEXT without FOR
```
