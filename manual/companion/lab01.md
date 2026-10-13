# Lab 1. Meet the Computer

> **THIS LAB:** Manual: Chapter 1. About 45 minutes. You need the manual's Figure 1-1 (or a real board to look at),
> and, for the last part, a Hydra or an emulator.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Find each of these on Figure 1-1, or on the board. Write its reference (like U1) and, in a few words, its job.

| Part | Reference | Its job |
|---|---|---|
| The processor | | |
| The crystal | | |
| The task RAM | | |
| The kernel ROM | | |
| The VIA | | |
| The serial chip (ACIA) | | |
| The sound chip | | |
| The socket for the Vera X | | |
| Where the power comes in | | |

2. Fill in the table. Work the answers out on paper (section 1.6).

| Decimal | Binary (8 bits) | Hex |
|---|---|---|
| 10 | | |
| 65 | | |
| 200 | | |
| | 0001 0000 | |
| | 1111 1111 | |
| | | $2A |
| | | $80 |

3. Now let the Hydra check your work. `calc` can write a number in any base: `-b b` is binary, `-b x` is hex, and
   a number that starts `#x` or `#b` is read in hex or binary. Try each, and correct your table.

```text
/> calc -b b 65
/> calc -b x 200
/> calc '#x2A'
/> calc '#b11111111'
```

### Questions

1. ✎ A computer does four kinds of thing: input, processing, memory and output. Give one part of the Hydra for each.

<div class="write l2"></div>

2. ✎ The Hydra is named for a monster with many heads. What are the Hydra-16's sixteen "heads"?

<div class="write l2"></div>

3. ✎ Each task has 32K of memory of its own. How many bytes is that? How much is that for all sixteen tasks, in K?

<div class="write l2"></div>

4. ✎ How many different values can one byte hold? Why that number?

<div class="write l2"></div>

5. ✎ What's the difference between RAM and ROM? Which one holds HydraOS, and why?

<div class="write l2"></div>

6. ✎ One hex digit stands for how many bits? So how many hex digits does it take to write a byte? An address?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | name the Hydra's main parts, and say what each is for |
| ☐ | turn a number from decimal into binary and hex, and back |
| ☐ | say what a `$` in front of a number means |

> **CHALLENGE:** ★ Write the year you were born in binary and in hex. Then: the Hydra's highest address is $FFFF.
> What is that in decimal? What would the highest address be if addresses had five hex digits?
