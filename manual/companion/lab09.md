# Lab 9. A Command Hunt

> **THIS LAB:** Manual: Chapter 11. About 45 minutes. You need a Hydra, and the manual open at Chapter 11. This
> lab is a hunt: the manual has the answers, and the Hydra will tell you whether you read it right.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Find the command for each job in Chapter 11, and write its name. (Each is one of the Hydra's programs.)

| The job | The command |
|---|---|
| Count the lines, words and bytes in a file | |
| Show the first few lines of a file | |
| Show the last few lines of a file | |
| Find the lines that have a word in them | |
| Put lines in order | |
| Show a file a screenful at a time | |
| Show a file's bytes in hex | |
| Say whether two files are the same | |
| Show how full the disks are | |
| Show which tasks are busiest, over and over | |
| Stop a program by its *name* | |
| Do nothing for a number of seconds | |

2. Now use them. The file to work on is `/sd/0/sample/as/hi.s`, a program's source. ✎ Write the command you
   typed, and the answer.

| Find out | The command I typed | The answer |
|---|---|---|
| How many lines it has | | |
| What its first three lines are | | |
| What its last line is | | |
| How many lines have the word `PRINT` in them | | |
| On which lines the word `window` appears (the line numbers) | | |

3. Make a small file to play with:

```text
/> echo pear >/ram/fruit
/> echo apple >>/ram/fruit
/> echo pear >>/ram/fruit
/> echo fig >>/ram/fruit
```

   ✎ Write the command, and what it printed.

| Find out | The command I typed | It printed |
|---|---|---|
| The fruit in alphabetical order | | |
| The same, backwards | | |
| The fruit in order, with each kind only once | | |
| How many bytes the file has | | |
| The file's bytes, in hex | | |

4. ✎ From the hex dump: what's the code for the letter `p`? What byte ends each line?

<div class="write"></div>

5. A usage line shows how to type a command. ✎ In the usage line `grep [-chilnsv] [-e] pattern [file ...]`, what
   do the square brackets mean? What does `...` mean? Which part must you always give?

<div class="write l2"></div>

### Questions

1. ✎ `ls` is a program in `/bin`, but `cd` isn't a program at all. What is it, and why can't it be a program?
   (The end of Chapter 11, and section 8.14.)

<div class="write l2"></div>

2. ✎ Many commands read their **standard input** if you don't name a file. Which of the commands above did you use
   that way, after a pipe?

<div class="write l2"></div>

3. ✎ What's the difference between `kill` and `slay`?

<div class="write"></div>

4. ✎ Which three commands in Chapter 11 do you think you'll use most? Why?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | find a command in Chapter 11, and read its usage line |
| ☐ | use `wc`, `head`, `tail`, `grep`, `sort` and `uniq` |
| ☐ | read a hex dump |
| ☐ | join two or three commands with pipes to answer a question |

> **CHALLENGE:** ★ Which letter do the most programs in `/rom/bin` start with? Work it out with the Hydra, not
> by counting on your fingers. (Patterns from Lab 5 will help: `ls /rom/bin/s* | wc -l`. Can a `for` loop from
> Lab 6 try every letter for you? To join a variable to a `*`, rc needs a `^` between them: `$l^*`.)
