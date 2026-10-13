## hash-create, to#

Functions: make a hash.

#### Syntax

```text
(hash-create [entries])
(to# entries)
#(entries)
```

#### Arguments

*entries*
: A list of entries. Each entry is a list: a key, a value, and after them any tags, `{key value tag ...}`.
  Without the argument, the hash is empty.

#### Result

A hash holding the entries, in the order given. It prints as `<hash>` and its entries.

#### Description

A **hash** is a table of values found by key. A key is an atom, a string, or an integer (a decimal with a
whole value, such as 3.0, is the integer). A value is anything, another hash or a function too.

Each entry's value is evaluated as the hash is made, so `{:legs (+ 2 2)}` stores 4; its key and its tags are
taken as written. If a key is given twice, the later entry replaces the earlier.

An item of the list that is an atom on its own, not an entry, is a tag on the hash itself (see
`hash-add-tag`).

`to#` is the same function under a shorter name, and `#(entries)` is another way to write the call.

A hash is small and searched from its first entry to its last: it is meant for records and objects, tens of
entries, not for thousands.

#### Errors

| Message | Cause |
|---|---|
| `A hash's entries are a list` | *entries* is not a list |
| `A hash's entry is {key value tag...}, not 1` | An item of the list is not an entry: usually one pair of braces is missing |
| `A hash key must be an atom, a string or an integer, not 1.5` | A key of another kind |

#### See also

`hash-put`, `hash-get`, `from#`, `hash-clone`.

#### Example

```session
hylang> (def {pet} (hash-create {{:name "Rex"} {:legs (+ 2 2)}}))
=> NIL
hylang> pet
=> <hash>{{:name "Rex"} {:legs 4}}
hylang> (len pet)
=> 2
hylang> #({{1 "one"} {"two" 2}})
=> <hash>{{1 "one"} {"two" 2}}
hylang> (hash-create {:a 1})
=> Error: A hash's entry is {key value tag...}, not 1
```

## from#

A function: a hash as a list.

#### Syntax

```text
(from# h)
```

#### Arguments

*h*
: A hash.

#### Result

A list of the hash's entries, each `{key value tag ...}`, in the order they were put, followed by the hash's
own tags. A value that is a hash is given as such a list too. The list is in the form `hash-create` takes, so
`(to# (from# h))` makes a hash like *h*.

#### Description

Every entry is in the list, private ones too, with their tags: `from#` is how a hash is examined, saved or
sent somewhere.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'from#' must be a hash` | *h* is not a hash |

#### See also

`hash-keys`, `hash-values`, `save`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (from# pet)
=> {{:name "Rex"} {:legs 4}}
hylang> (map fst (from# pet))
=> {:name :legs}
```

## hash-get

A function: the value at a key of a hash. Calling the hash itself does the same.

#### Syntax

```text
(hash-get h key)
(h key)
```

#### Arguments

*h*
: A hash.

*key*
: An atom, a string or an integer.

#### Result

The value stored at the key, or `NIL` if the hash has no such key. Since a stored value can itself be `NIL`,
use `hash-key?` to tell the two apart.

#### Description

A hash can stand where a function does: `(h key)` looks the key up. The two forms differ only when the value
is a function. `hash-get` gives the function as a value. `(h key)` calls it, as a method, with `&0` bound to
the hash (see `hash-call`).

`(h)`, with no key, is the hash itself. `hash-get` of `NIL` is `NIL`, so a lookup in a hash that was never
made does not stop a program.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-get' must be a hash` | *h* is neither a hash nor `NIL` |
| `hash-get error: cannot access private hash entry` | The entry is private (see `hash-make-private`): only the hash's own methods can read it |

#### See also

`hash-put`, `hash-key?`, `hash-call`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (pet :name)
=> "Rex"
hylang> (hash-get pet :legs)
=> 4
hylang> (pet :wings)
=> NIL
```

## hash-put

A function: add entries to a hash, or change them.

#### Syntax

```text
(hash-put h {key value} ...)
```

#### Arguments

*h*
: A hash.

*{key value}*
: An entry, as `hash-create` takes: the value is evaluated, and tags may follow it. There may be several
  entries.

#### Result

The value the key had before, or `NIL` if the key is new. With several entries, a list of those values, one
for each.

#### Description

The hash itself is changed: every name bound to it sees the change. An entry whose key is already there
takes the old one's place in the order; a new key goes at the end.

Each entry needs its own braces. `(hash-put h :a 1)` is not an entry, and gives errors for `:a` and for 1.

#### Errors

| Message | Cause |
|---|---|
| `A hash's entry is {key value tag...}, not :a` | An argument is not an entry |
| `hash-put error: cannot add new entries to a locked hash` | The hash is locked, and the key is new (see `hash-lock`) |
| `hash-put error: cannot modify read-only hash` | The hash is constant (see `hash-make-const`) |
| `hash-put error: cannot modify read-only hash entry` | That entry is constant |
| `hash-put error: cannot access private hash entry` | The entry is private, and this is not one of the hash's methods |
| `Cannot set non-nillable field :v to NIL.` | The entry must not be `NIL` (see `hash-make-not-nil`) |

#### See also

`hash-get`, `hash-remove`, `hash-create`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-put pet {:legs 3})
=> 4
hylang> (hash-put pet {:colour "brown"} {:age 2})
=> {NIL NIL}
hylang> (hash-keys pet)
=> {:name :legs :colour :age}
```

## hash-key?, hash-keys, hash-values

Functions: is a key in a hash? And all its keys, and all its values.

#### Syntax

```text
(hash-key? h key)
(hash-keys h)
(hash-values h)
```

#### Arguments

*h*
: A hash.

*key*
: An atom, a string or an integer.

#### Result

`hash-key?` gives `T` if the hash has an entry with that key, whatever its value, and `NIL` if not.
`hash-keys` gives a list of the keys and `hash-values` a list of the values, both in the order the entries
were put, so the two lists match item for item. For an empty hash each is `NIL`.

Private entries are listed with the rest.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-keys' must be a hash` | *h* is not a hash (each function with its own name) |

#### See also

`from#`, `hash-get`, `len`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-key? pet :name)
=> T
hylang> (hash-key? pet :wings)
=> NIL
hylang> (hash-keys pet)
=> {:name :legs}
hylang> (hash-values pet)
=> {"Rex" 4}
```

## hash-remove

A function: take an entry out of a hash.

#### Syntax

```text
(hash-remove h key)
```

#### Arguments

*h*
: A hash.

*key*
: The entry's key.

#### Result

The value the entry had. If there was no such key, `NIL`, and nothing is changed.

#### Errors

| Message | Cause |
|---|---|
| `hash-remove error: cannot remove an entry from a locked hash` | The hash is locked, or the entry is constant |

#### See also

`hash-put`, `hash-lock`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (hash-remove pet :legs)
=> 4
hylang> (hash-remove pet :legs)
=> NIL
hylang> (hash-keys pet)
=> {:name}
```

## hash-call

A function: call a method of a hash. Calling the hash with the key and arguments does the same.

#### Syntax

```text
(hash-call h key x ...)
(h key x ...)
```

#### Arguments

*h*
: A hash.

*key*
: The key of an entry whose value is a function.

*x*
: The function's arguments.

#### Result

What the function returns.

#### Description

A function stored in a hash is a **method**. When it is called through the hash, the symbol `&0` is bound,
inside it, to the hash, so the method can read and change the hash's other entries: `(&0 :n)`,
`(hash-put &0 {:n 1})`. A hash with data and methods is an object, and `hash-clone` makes more like it.

Through `&0` a method reaches every entry, private ones too. That is what a private entry is: one that only
the hash's methods can use.

The short form `(h key x ...)` is the usual way to write the call. With no arguments after the key it still
calls the method: `(counter :inc)`. To get the function itself, without calling it, use `hash-get`.

Given fewer arguments than it has formals, a method, like any function, returns a function of the rest.

#### Errors

| Message | Cause |
|---|---|
| `Second parameter to 'hash-call' must be a key to a member function` | The key isn't there, or its value isn't a function |
| `:v isn't a method: its value isn't a function` | The short form, with arguments after a key whose value isn't a function |
| `First parameter to 'hash-call' must be a hash` | *h* is not a hash |

#### See also

`hash-get`, `hash-clone`, `hash-make-private`, `fn`.

#### Example

```session
hylang> (def {counter} #({{:n 0} {:inc (fn {} {hash-put &0 {:n (1+ (&0 :n))}})} {:plus (fn {x} {+ x (&0 :n)})}}))
=> NIL
hylang> (counter :inc)
=> 0
hylang> (counter :inc)
=> 1
hylang> (counter :n)
=> 2
hylang> (counter :plus 10)
=> 12
hylang> (hash-call counter :plus 10)
=> 12
```

## hash-clone

A function: a copy of a hash.

#### Syntax

```text
(hash-clone h [override ...])
```

#### Arguments

*h*
: A hash.

*override*
: An entry, `{key value tag ...}`, to put in the copy: it replaces the entry of that key, or is added. An
  atom on its own is a tag for the copy. Unlike `hash-create`'s, an override's value is taken as it is
  written, not evaluated.

#### Result

A new hash with the same entries and tags, and then the overrides. The two are separate from then on: a
change to one is not seen in the other. A value that is itself a hash is copied too, so the copy shares
nothing with the original.

#### Description

This is how one object is made from another: clone a hash that has the methods and the usual values, giving
the entries that differ.

#### Errors

| Message | Cause |
|---|---|
| `First parameter to 'hash-clone' must be a hash` | *h* is not a hash |

#### See also

`hash-create`, `hash-call`.

#### Example

```session
hylang> (def {pet} #({{:name "Rex"} {:legs 4}}))
=> NIL
hylang> (def {twin} (hash-clone pet {:name "Max"}))
=> NIL
hylang> (twin :name)
=> "Max"
hylang> (pet :name)
=> "Rex"
hylang> (hash-put twin {:legs 3})
=> 4
hylang> (pet :legs)
=> 4
```

## hash-add-tag, hash-tag?

Functions: mark a hash, or one of its entries, with a tag; and test for one.

#### Syntax

```text
(hash-add-tag h tag ...)
(hash-add-tag h {key tag} ...)
(hash-tag? h tag)
(hash-tag? h {key tag})
```

#### Arguments

*h*
: A hash.

*tag*
: An atom: a tag for the hash itself.

*{key tag}*
: A key and an atom: a tag for that key's entry.

#### Result

`hash-add-tag` gives `T` if the tag was added, and `NIL` if it was already there. With several tags, a list
of those answers. `hash-tag?` gives `T` if the hash, or the entry, has the tag, and `NIL` if not.

#### Description

A **tag** is a label a program puts on a hash or on an entry, to mean whatever the program wants: a kind of
record, a field to be saved, a unit. Tags are no part of the values. They show after an entry's value, and
after the last entry for the hash's own, when the hash is printed or listed with `from#`, and they can be
given in the same places when a hash is made.

Four tags are the system's, and change what can be done with a hash: `hash-lock` and the functions beside it
put them on.

#### Errors

| Message | Cause |
|---|---|
| `Failed to add tag: invalid parameters "notatom"` | A tag is not an atom, or the list is not a key and one tag |
| `hash-add-tag failed: entry not found` | The key is not in the hash |

#### See also

`hash-lock`, `hash-locked?`, `from#`.

#### Example

```session
hylang> (def {h} #({{:a 1} {:b 2}}))
=> NIL
hylang> (hash-add-tag h :big)
=> T
hylang> (hash-add-tag h {:a :metres})
=> T
hylang> h
=> <hash>{{:a 1 :metres} {:b 2} :big}
hylang> (hash-tag? h :big)
=> T
hylang> (hash-tag? h {:b :metres})
=> NIL
```

## hash-lock, hash-make-const, hash-make-private, hash-make-not-nil

Functions: protect a hash, or some of its entries.

#### Syntax

```text
(hash-lock h [key ...])
(hash-make-const h [key ...])
(hash-make-private h [key ...])
(hash-make-not-nil h [key ...])
```

#### Arguments

*h*
: A hash.

*key*
: The keys of the entries to protect. With none, the protection is put on the hash as a whole.

#### Result

`T` if the protection was added, `NIL` if it was already there; a list of those for several keys.

#### Description

Each puts one of the system's four tags on the hash or on the entries named, and the tag changes what
`hash-put`, `hash-get` and `hash-remove` will do:

| Function | Tag | On the hash | On an entry |
|---|---|---|---|
| `hash-lock` | `:__locked` | No entry can be added or removed. The values of the entries it has can still be changed | |
| `hash-make-const` | `:__read-only` | Nothing can be changed, added or removed | The entry can't be changed or removed |
| `hash-make-private` | `:__private` | | The entry can be read and changed only by the hash's own methods, through `&0` |
| `hash-make-not-nil` | `:__not_nil` | | The entry can't be given the value `NIL` (it can still be removed) |

A protection, once put on, stays: there is no function that takes one off. A copy made with `from#` and
`to#` carries the tags with it.

A private entry is hidden from `hash-get` and from `hash-put`, but not from `hash-keys`, `hash-values` and
`from#`, which list it with the rest: the protection is against accidents, not against a determined program.

The same tags can be written in an entry as a hash is made: `{:a 1 :__read-only}`.

#### Errors

| Message | Cause |
|---|---|
| `hash-add-tag failed: entry not found` | A key is not in the hash |
| `First parameter to 'hash-lock' must be a hash` | *h* is not a hash (each function with its own name) |

The errors that a protection causes are listed under `hash-put`, `hash-get` and `hash-remove`.

#### See also

`hash-locked?`, `hash-add-tag`, `hash-call`.

#### Example

```session
hylang> (def {acct} #({{:owner "Ann"} {:pin 1234} {:check (fn {n} {== n (&0 :pin)})}}))
=> NIL
hylang> (hash-make-private acct :pin)
=> T
hylang> (acct :pin)
=> Error: hash-get error: cannot access private hash entry
hylang> (acct :check 1234)
=> T
hylang> (hash-make-const acct :owner)
=> T
hylang> (hash-put acct {:owner "Bob"})
=> Error: hash-put error: cannot modify read-only hash entry
hylang> (hash-lock acct)
=> T
hylang> (hash-put acct {:extra 1})
=> Error: hash-put error: cannot add new entries to a locked hash
```

## hash-locked?, hash-private?, hash-const?

Functions: is a hash, or one of its entries, protected?

#### Syntax

```text
(hash-locked? h [key])
(hash-private? h [key])
(hash-const? h [key])
```

#### Arguments

*h*
: A hash.

*key*
: An entry's key. Without it, the question is about the hash as a whole.

#### Result

`T` if the hash, or the entry, has the protection that `hash-lock`, `hash-make-private` or
`hash-make-const` puts on, and `NIL` if not.

The hash and its entries are separate: `(hash-const? h)` is `NIL` for a hash that has constant entries but
was not itself made constant.

#### Errors

| Message | Cause |
|---|---|
| `Key :v not found when looking up tag for hash entry` | The key is not in the hash |

#### See also

`hash-lock`, `hash-tag?`.

#### Example

```session
hylang> (def {k} #({{:a 1} {:b 2}}))
=> NIL
hylang> (hash-make-const k :a)
=> T
hylang> (hash-const? k :a)
=> T
hylang> (hash-const? k :b)
=> NIL
hylang> (hash-const? k)
=> NIL
hylang> (hash-locked? k)
=> NIL
```
