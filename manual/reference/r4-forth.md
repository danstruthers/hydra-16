# Chapter 4. HyForth Words

A cell is 16 bits. A double is two cells, 32 bits, its high cell on top of the stack. A flag is 0 for false and
−1 (all bits set) for true. *ior* is 0 for success and a non-zero error code otherwise. Stack pictures follow the
Forth standard's notation: *n* signed, *u* unsigned, *d* and *ud* doubles, *c-addr u* the address and length of
a string.

Each entry says which word set a word belongs to, and whether it is in the ROM core or needs a library loaded
with `lib` first. Where a word is the standard's, the entry describes what HyForth does in the cases the
standard leaves open.

## >number

Core word set, in the ROM core. Converts the digits at the start of a string, adding them to a double.

#### Syntax

```text
>number  ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
```

#### Arguments

*ud1*
: The number so far: 0 to start a new one.

*c-addr1 u1*
: The text to convert.

#### Result

*ud2*
: *ud1* with each converted digit added: for every digit, *ud* × `base` + the digit's value.

*c-addr2 u2*
: What was not converted: the address of the first character that isn't a digit and the number of characters
  from there to the end. *u2* is 0 if the whole string was digits, and *c-addr2* is then the address just past
  it.

#### Description

Characters are taken from the left for as long as each is a digit in the current `base`. The digits are `0` to
`9`, then the letters `A` to `Z` for 10 to 35; small letters count as capitals. A character whose value is not
less than `base` is not a digit, and conversion stops at it: in decimal `a` stops it, in hex `g` does.

Nothing else is understood. A sign, a prefix (`$`, `#` or `%`) and a decimal point all stop the conversion like
any other non-digit; an empty string converts nothing and returns *ud1* unchanged. Signs and prefixes are the
text interpreter's business: it deals with them itself, and calls this word's code for the digits.

#### Errors

None. A string with no digits is not an error: *ud2* = *ud1* and *u2* = *u1*.

#### Notes

A value that passes 32 bits wraps: only its low 32 bits are kept, and no error is raised. (The standard calls
this an ambiguous condition.) To read a number of any length, use the numbers library (`lib numbers`).

`base` can be 2 to 36.

#### See also

`base`, `decimal`, `hex`, `s>d`, `evaluate`.

#### Example

`nip` drops the address, leaving the double (its low cell first, as `.s` shows it) and the count of characters
not converted:

```session
/> 0 0 s" 123abc" >number nip .s
<3> 123 0 3
/> drop 2drop
/> hex 0 0 s" ff zz" >number nip .s decimal
<3> FF 0 3
/> drop 2drop
/> 0 0 s" -5" >number nip .s
<3> 0 0 2
/> drop 2drop
/> 0 0 s" 99999999999" >number nip .s
<3> 2839 -20518 0
```
