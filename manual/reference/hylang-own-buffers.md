## buffer

A function: makes a buffer, a block of bytes that can be changed in place.

#### Syntax

```text
(buffer n [fill])
(buffer s)
(buffer l)
(buffer b)
```

#### Arguments

*n*
: The buffer's size in bytes: a whole number from 0 to 8,184.

*fill*
: The value every byte starts with: 0 to 255, or a character. Without it, 0.

*s*
: A string: the buffer holds its bytes.

*l*
: A list of bytes, each a whole number from 0 to 255 or a character.

*b*
: Another buffer: the result is a copy of it.

#### Result

A buffer. It prints as `<buffer>` and its bytes.

#### Description

A **buffer** is a row of bytes of a fixed length. Unlike a string or a list, it is changed where it is:
`buffer-put`, `buffer-fill`, `buffer-copy` and `read-buffer` alter the buffer they are given, and every
name bound to that buffer sees the change. That makes it the value to use for binary data, for a file read a
block at a time, and for a table that is updated many times, where making a new list for each change would
be slow.

A buffer has a length (`len`), is equal to another buffer with the same bytes (`==`), and can be called with
an index, `(b i)`, to read a byte. `bytes` gives its bytes as a list and `from-bytes` as a string. It is not
a list: `map`, `nth` and the other list functions do not take one.

#### Errors

| Message | Cause |
|---|---|
| `'buffer': -1 bytes is outside 0 to 8184` | *n* is negative, or more than a buffer can hold |
| `'buffer': a size must be a whole number, not 1.5` | The argument is not a size, a string, a list or a buffer |
| `'buffer': 256 isn't a byte (0-255)` | *fill*, or an item of the list, is not a byte |

#### See also

`buffer-get`, `buffer-fill`, `read-buffer`, `write-bytes`, `bytes`, `from-bytes`.

#### Example

```session
hylang> (buffer 4)
=> <buffer>{0 0 0 0}
hylang> (buffer 4 255)
=> <buffer>{255 255 255 255}
hylang> (buffer "Hi")
=> <buffer>{72 105}
hylang> (buffer {1 2 3})
=> <buffer>{1 2 3}
hylang> (len (buffer 100))
=> 100
hylang> (from-bytes (buffer {72 105}))
=> "Hi"
```

## buffer?

A function: is a value a buffer?

#### Syntax

```text
(buffer? x)
```

#### Arguments

*x*
: Any value.

#### Result

`T` if *x* is a buffer, and `NIL` if it is anything else: a list of bytes is not a buffer, and nor is a
string.

#### See also

`buffer`, `type-of`.

#### Example

```session
hylang> (buffer? (buffer 4))
=> T
hylang> (buffer? {1 2})
=> NIL
```

## buffer-get, buffer-put

Functions: read one byte of a buffer; and set one.

#### Syntax

```text
(buffer-get b i)
(b i)
(buffer-put b i byte)
```

#### Arguments

*b*
: A buffer.

*i*
: The byte's position, from 0 to one less than the buffer's length.

*byte*
: The new value: a whole number from 0 to 255, or a character, whose code is stored.

#### Result

`buffer-get` gives the byte at *i*, a number from 0 to 255. Calling the buffer itself, `(b i)`, does the
same.

`buffer-put` stores *byte* at *i*, in the buffer itself, and gives the byte that was there before.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-get': index 4 is outside the buffer` | *i* is negative, or the buffer's length or more (and `'buffer-put'`) |
| `'buffer-get': an index must be a whole number, not 1.5` | *i* is not a whole number |
| `'buffer-put': 256 isn't a byte (0-255)` | *byte* is not a byte |
| `'buffer-get' expects a buffer, not {1}` | *b* is not a buffer |

#### See also

`buffer`, `buffer-fill`, `peek`, `poke`.

#### Example

```session
hylang> (def {b} (buffer 4))
=> NIL
hylang> (buffer-put b 0 65)
=> 0
hylang> (buffer-put b 0 66)
=> 65
hylang> (b 0)
=> 66
hylang> (buffer-get b 1)
=> 0
hylang> (buffer-get b 4)
=> Error: 'buffer-get': index 4 is outside the buffer
```

## buffer-fill, buffer-copy

Functions: set many bytes of a buffer to one value; and copy bytes from one buffer to another.

#### Syntax

```text
(buffer-fill b byte [i [n]])
(buffer-copy to at from [i [n]])
```

#### Arguments

*b*
: The buffer to fill.

*byte*
: The value to fill with: 0 to 255, or a character.

*to*, *at*
: The buffer to copy into, and the position in it where the first byte goes.

*from*
: The buffer to copy from.

*i*
: The position of the first byte to fill, or to copy from *from*. Without it, 0.

*n*
: How many bytes. Without it, all the bytes from *i* to the end.

#### Result

`NIL`. The buffer *b*, or *to*, is changed in place.

#### Description

A count that goes past the end of a buffer is not an error: the fill stops at the end of *b*, and the copy
stops when *from* runs out or *to* is full, whichever comes first.

*to* and *from* may be the same buffer, and the two parts may overlap: the bytes are copied as if they were
all read first.

Both arguments of `buffer-copy` must be buffers. To put a string's or a list's bytes in a buffer, make a
buffer of them first: `(buffer-copy b 0 (buffer "xy"))`.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-fill': index 5 is outside the buffer` | *i*, or *at*, is negative or past the end |
| `'buffer-fill': 300 isn't a byte (0-255)` | *byte* is not a byte |
| `'buffer-copy' expects a buffer, not xy` | *to* or *from* is not a buffer |

#### See also

`buffer`, `buffer-put`, `read-buffer`.

#### Example

```session
hylang> (def {b} (buffer 6))
=> NIL
hylang> (buffer-fill b 7)
=> NIL
hylang> (buffer-fill b 1 4)
=> NIL
hylang> b
=> <buffer>{7 7 7 7 1 1}
hylang> (buffer-copy b 1 (buffer "abcdef") 2 3)
=> NIL
hylang> b
=> <buffer>{7 99 100 101 1 1}
```

## read-buffer

A function: reads bytes from a stream into a buffer.

#### Syntax

```text
(read-buffer s b [at [n]])
```

#### Arguments

*s*
: A stream open for reading.

*b*
: The buffer to read into.

*at*
: Where in the buffer the first byte goes. Without it, 0.

*n*
: The most bytes to read. Without it, as many as fit from *at* to the end of the buffer.

#### Result

The number of bytes read, which is less than was asked for when the file has fewer left; and `NIL` at the
end of the file. The bytes are in the buffer from *at* on, and the rest of the buffer is as it was.

#### Description

This is the way to read a file a block at a time without making a new value for each block: one buffer is
used over and over, and the loop ends when the result is `NIL`. `write-bytes` writes a buffer, or a part of
one, back to a stream.

#### Errors

| Message | Cause |
|---|---|
| `'read-buffer' expects a stream` | *s* is not a stream |
| `Cannot read from this stream` | The stream is not open for reading |

#### See also

`read-bytes`, `write-bytes`, `open`, `buffer`.

#### Example

```session
hylang> (write-file "/ram/bf" "0123456789")
=> NIL
hylang> (def {s} (open "/ram/bf"))
=> NIL
hylang> (def {r} (buffer 4))
=> NIL
hylang> (read-buffer s r)
=> 4
hylang> r
=> <buffer>{48 49 50 51}
hylang> (read-buffer s r)
=> 4
hylang> (read-buffer s r)
=> 2
hylang> (read-buffer s r)
=> NIL
hylang> (close s)
=> NIL
```

## buffer-cmp

A function of the library, written in hylang: compares part of a buffer with a string, or with another
buffer.

#### Syntax

```text
(buffer-cmp b at x [i [n]])
```

#### Arguments

*b*
: A buffer.

*at*
: Where in *b* the comparison starts.

*x*
: What to compare with: a string, or a buffer.

*i*
: Where in *x* to start. Without it, 0.

*n*
: How many bytes of *x* to compare. Without it, all of them from *i* on.

#### Result

-1, 0 or 1, as `cmp` gives for two strings: is the part of *b* before, the same as, or after the part of
*x*?

#### Description

At most as many bytes of *b* are looked at as *x* has. So the result is 0 when *b*, from *at*, *begins*
with *x*: this is the test for "is this text here in the buffer?", such as a file's signature at its start.
If *b* ends before *x* does, and they agree as far as *b* goes, the result is -1.

#### Errors

| Message | Cause |
|---|---|
| `'buffer-cmp' expects a buffer, not 5` | *b* is not a buffer |
| `'buffer-cmp' expects a buffer or a string, not 5` | *x* is neither |
| `'buffer-cmp': index 5 is outside the buffer` | *at* or *i* is negative, or past the end |

#### See also

`cmp`, `==`, `index-of`.

#### Example

```session
hylang> (def {b} (buffer "HYX2 and more"))
=> NIL
hylang> (buffer-cmp b 0 "HYX2")
=> 0
hylang> (buffer-cmp b 0 "HYX3")
=> -1
hylang> (buffer-cmp b 5 "and")
=> 0
```
