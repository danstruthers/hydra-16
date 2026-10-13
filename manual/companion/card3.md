# Card 3. rc

The command language (Chapter 8). At the `/>` prompt, a line that doesn't start with a Forth word goes to rc; `%`
and a space in front makes sure of it; and `rc` starts rc itself, with the prompt `%`.

### Words and quotes

| Written | Means |
|---|---|
| `'it''s here'` | One word, exactly as written (two `'` inside for one) |
| `a^b` | The two joined: `$name^.txt` |
| `# text` | A comment |
| *command* `;` *command* | One after the other |
| `{` *commands* `}` | Several commands as one |

### Variables

| Written | Means |
|---|---|
| `x=hello`, `x=(a b c)` | Give a variable a word, or a list of words |
| `$x` | Its value: every word of it |
| `$x(2)` | Its second word |
| `$#x` | How many words it has |
| `$"x` | All its words as one word |
| `$*`, `$1`, `$#*`, `$0` | In a script or function: its arguments; the first; how many; its name |
| `$status` | How the last command ended: empty means it worked |
| `` `{`` *command* ``}`` | The command's output, as words |

### Files and patterns

| Written | Means |
|---|---|
| `>`*file*, `>>`*file*, `<`*file* | Output to a file; added to its end; input from one |
| `>[2]`*file* | Error messages to a file (`/dev/null` throws them away) |
| `*` | Any characters: `*.bas` |
| `?` | Any one character |
| `[abc]`, `[a-z]` | Any one of these |

### Decisions and loops

```text
if(~ $x hello) echo yes          # ~ tests a word against patterns
if not echo no                   # the "else" of the if just before

for(f in *.txt) echo $f          # once for each word

while(~ $x hello) { ... }        # as long as the command succeeds

switch($x){
case h*
	echo starts with h
case *
	echo something else
}
```

| Written | Means |
|---|---|
| `~` *word pattern* ... | Succeeds if the word matches any of the patterns |
| `!` *command* | Succeeds if the command fails |
| *a* `&&` *b*, *a* `\|\|` *b* | Do *b* if *a* worked; if *a* failed |

### Functions and scripts

```text
fn greet { echo Hello, $1! }     # a new command, until rc ends

#!/bin/rc                        # a script: a file of commands.
echo I was given $#* words: $*   # Run it with rc file, or copy it to /bin
```

### Built in (section 8.14)

| Command | What it does |
|---|---|
| `cd` *dir* | Change directory |
| `exit` | Leave rc |
| `whatis` *name* | Say what a name is: a program, a function, a variable |
| `.` *file* | Run a file's commands in this rc, keeping what they set |
| `bind`, `mount`, `unmount` | Change the namespace (Chapter 9) |
| `wait` | Wait for background commands to end |
