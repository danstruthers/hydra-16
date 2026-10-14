# Card 2. Everyday Commands

Chapter 11 has every command, with its options. These are the ones you'll use most.

### Looking

| Command | What it does |
|---|---|
| `ls` [`-l`] [*dir*] | List a directory (`-l`: with sizes and dates) |
| `pwd` | Say which directory you're in |
| `cd` [*dir*] | Go to a directory (`cd ..`: up one; `cd /`: to the top) |
| `cat` *file* | Show a file |
| `more` *file* | Show it a screenful at a time (<kbd>Q</kbd> and <kbd>RETURN</kbd> leave) |
| `head -5` *file*, `tail -5` *file* | Its first five lines; its last five |
| `xd` *file* | Its bytes, in hex |

### Changing

| Command | What it does |
|---|---|
| `edit` *file* | Make or change a file, in the screen editor |
| `cp` *from to* | Copy a file |
| `mv` *from to* | Move it, or rename it |
| `rm` *file* | Remove it, for good |
| `mkdir` *dir*, `rmdir` *dir* | Make a directory; remove an empty one |
| `echo` *words* `>`*file* | Put a line in a file (`>>`: add a line to its end) |

### Finding out

| Command | What it does |
|---|---|
| `wc` [`-l`] *file* | Count its lines, words and bytes (`-l`: just lines) |
| `grep` *word file* | Show the lines with a word in them (`-n`: with line numbers; `-c`: just count them) |
| `sort` [`-r`] *file* | Its lines in order (`-r`: backwards) |
| `cmp` *file1 file2* | Say whether two files are the same |
| `df`, `du` *dir* | How full the disks are; how much a directory holds |
| `date` | The date and time |
| `free` | How much memory there is |
| `calc` `'`*sum*`'` | Work out a sum: `calc '2^64'`, `calc -b x 255` |

### Tasks

| Command | What it does |
|---|---|
| `ps` | List the tasks |
| `top` | Show the busiest tasks, again and again (<kbd>CTRL</kbd>-<kbd>C</kbd> stops it) |
| *command* `&` | Start a command in the background |
| `kill` *task*, `slay` *name* | Stop a task by its number; by its name |
| `sleep` *seconds* | Wait |

### Languages and tools

| Command | What it does |
|---|---|
| `basic` [*file*], `hylang` [*file*], `forth` [*file*] | Start a language, or run a program in it |
| `rc` [*file*] | Start rc, or run a script |
| `play` *song*, `play -m 0 '`*mml*`'` | Play a song file; play a line of music |
| `as` *file*`.s`, `dis` *program*, `db` *program* | Assemble; disassemble; debug |

### Joining commands (Chapter 8)

| Written | Means |
|---|---|
| *command* `>`*file* | Send its output to a file |
| *command* `<`*file* | Take its input from a file |
| *command* `\|` *command* | Send the first one's output into the second (nine at most, on the book's Hydra) |
| *command* `&&` *command* | Do the second only if the first worked |
| *command* `\|\|` *command* | Do the second only if the first failed |

### Where things are (section 6.5)

| Directory | What's there |
|---|---|
| `/sd/0` | The SD card: **keep your work here**, in `/sd/0/work` |
| `/ram` | A RAM disk: quick, and emptied at every reset |
| `/rom` | HydraOS's own files: read, but never changed |
| `/bin` | Every command |
| `/dev` | The devices, as files |
