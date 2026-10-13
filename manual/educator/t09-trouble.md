# Topic 9. When Things Go Wrong in Class

Things that will happen with thirty students and thirty Hydras, and what to do about each, quickly. Hardware
faults in a board you've built are the manual's section 5.7 and Topic 6; this topic is about the lesson.

## The emulator

| What happens | Why | What to do |
|---|---|---|
| The page opens, but nothing appears in the console | The browser blocked the page's scripts, or it's a very old browser | Try another browser. Some school set-ups block a page opened from a network drive: copy it to the computer's own disk, and try again |
| A student's files have gone since last lesson | Another computer, another login, or the browser's storage was cleared (Topic 3) | Nothing brings them back. **Save card** at the end of every lesson, from now on |
| Typing does nothing | The console doesn't have the keyboard | Click in the console |
| <kbd>CTRL</kbd>-<kbd>W</kbd> closed the page; <kbd>CTRL</kbd>-<kbd>N</kbd> opened a window | The browser took the key before the page saw it | There's no cure inside a browser tab. Find out beforehand which keys your browser takes, and warn the class; or use the terminal emulator, which doesn't have the problem |
| No sound | The **Sound** button hasn't been pressed; or the speed isn't **Real time**; or the computer is muted | In that order |
| Music plays too fast | The speed is 2×, 4× or **As fast as it goes** | **Real time** |
| It's very slow | An old computer running at **Real time** can't keep up: the page shows how close to real time it's running | Close other tabs. Turn off the Vera X in Setup if the lab doesn't need it |
| The start-up messages don't match the book | The Vera X and the clock chip (the manual's "This book's Hydra") | Nothing. Tell them it's expected |
| A student pressed **New** on their SD card | It's empty now | **Samples** brings the sample card back (and loses what was there). **Load…** brings back a saved card |

## At the prompt

| What a student says | What's happened | What to do |
|---|---|---|
| "Nothing I type does anything" | A program is running and waiting; or they're inside a HyForth definition (a prompt of spaces); or inside a quote | <kbd>CTRL</kbd>-<kbd>C</kbd>; then <kbd>CTRL</kbd>-<kbd>D</kbd>; then `;`; then reset. In that order |
| "It says not found" | A misspelling; a capital; or the file is in another directory | "Read me what you typed." Then `ls`, to see what *is* there |
| "My prompt looks different" | They're in rc (`%`), HyBASIC (`>`), hylang, `ed` (`*`) or `db` | Card 1's table: each prompt, and how to leave |
| "It printed a question mark" | HyForth doesn't know a word: usually a missing space | Look for two things run together: `3+`, `."Hello` |
| "My program has gone" | It was in `/ram`, and the Hydra was reset; or it was only in HyBASIC's memory, never saved | Sympathy, and then the rule: `/sd/0/work` |
| "`edit` won't let me out" | <kbd>CTRL</kbd>-<kbd>C</kbd> doesn't leave the editor, on purpose | <kbd>CTRL</kbd>-<kbd>X</kbd>, then <kbd>Y</kbd> or <kbd>N</kbd> |
| "The screen is a mess" | A program printed binary, or two tasks wrote at once | <kbd>CTRL</kbd>-<kbd>L</kbd> redraws the editor. Otherwise open a new window (<kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>C</kbd>), or reset: a reset cures everything |
| "It says `no free task`" | All sixteen tasks are in use: a pipeline of seven programs (the manual's section 8.7), or programs left running in other windows | `ps` shows them. Use fewer programs in the line, or end what's still running (`kill`) |
| "It says `duplicate definition`" | A HyBASIC program with two `SUB`s or `FUNCTION`s of one name (section 23.13) | `LIST`, find the two, and rename one |
| "I pasted my program and bits of it are missing" | The terminal program's flow control isn't XON/XOFF, so it didn't wait when the Hydra asked (the manual's sections 5.2 and 13.6) | Set it to XON/XOFF. Or paste a screenful at a time; or bring the file over with `/pc` |
| "The date is wrong" | No clock chip: it's the year 2000 | `echo` the date into `/dev/time` (Lab 14), or ignore it |
| "It's stuck for ever" | A loop that doesn't end | <kbd>CTRL</kbd>-<kbd>C</kbd>. Then ask them which line was supposed to stop it |

## A real Hydra

| What happens | What to do |
|---|---|
| Nothing on the terminal after the power goes on | The port and 9600 baud, first. Then press reset. Then the manual's section 5.7, from the top |
| Nonsense on the terminal | The baud rate: someone changed `/dev/serctl` last lesson, and the terminal program remembers the old speed. The Hydra always starts at 9600 |
| It was working, and now it resets by itself | Something on the I/O header is shorting the supply; or a chip has worked loose. Unplug whatever was added last |
| `/sd/0` isn't there | The card isn't pushed in; the module's wires have come off J18; or it's a new card with no HydraFS (section 14.2) |
| The SD card "lost" a file | The power went off while a program was writing. `fsck 0`, and `fsck -f 0` to repair (section 14.6). Teach: leave the editor before switching off |
| A student has changed something and now it won't start | The settings live on the SD card, in `/sd/0/lib`. Take the card out, and the Hydra starts from its ROM. Then fix the file from another Hydra, or from the PC with `hydrafs.js` |

## The lesson itself

**The projector Hydra fails in front of the class.** It will, one day. Narrate what you do: "the prompt hasn't
come back, so first I try <kbd>CTRL</kbd>-<kbd>C</kbd>..." You're modelling exactly the behaviour you want from
them. A teacher calmly recovering from a mistake teaches more than a flawless demonstration.

**Half the class has finished, and half hasn't started.** Stop everyone. Have a finished student show step 1 at
the projector. Start again from there, with the finished half on the challenge.

**One student's problem is eating the lesson.** Give it two minutes. If it isn't solved: reset their Hydra, have
them start the step again, and write the problem on the board as a puzzle for anyone who finishes early. Most
"impossible" problems are a typing slip three steps back, and a clean start is quicker than archaeology.

**Nobody did the reading.** Don't lecture it instead: that teaches them not to read. Give ten minutes of silent
reading now, of the two sections that matter most, and cut the lab's last steps.

**The computers are down.** Lab 1 (to step 2) and Lab 24 need only paper. So does planning a project: have them
write, by hand, the program they mean to type next lesson. They'll type it faster and it will work sooner.

**A student asks something you can't answer.** Say so, and say "let's find out": the manual's index, then the
Hydra itself. With this machine, more than most, the answer really is in there, and looking for it together is
the best lesson of the week.
