# Topic 8. Reaching Everyone

Every class has a student who finished Lab 3 in ten minutes and one who is still looking for the <kbd>CTRL</kbd>
key. The Hydra is unusually good at holding both, because there is always a level below to go down to, and always
a chapter that can wait. This topic is about using that.

## The student who races ahead

Don't give them more of the same. Give them something deeper.

* **Every lab's challenge** (the ★ box) is written to be genuinely harder, not just longer. Some of them (primes
  in HyForth, the Hydra guessing *your* number) will hold a strong student for a lesson.
* **Send them down a layer.** Finished the HyBASIC labs? Read Chapter 26, and do Lab 24. The quick student in a
  HyBASIC class who turns up with a working `ASM` block has learned more than six extra BASIC exercises would
  teach.
* **Send them sideways.** A second language (Lab 23's challenge: do it again in another) is real stretch, and it
  can't be done by typing faster.
* **Make them the expert.** One student who knows `db`, or the window keys, or MML, and who the others must ask.
  Explaining is the best test of understanding there is, and it frees you.
* **The source.** For the rare student who wants it: HydraOS's whole source is in the repository, with comments,
  and the manual's Appendix K lists the system calls it implements. "Find where `ps` gets its list from" is a
  week's happy digging.

> **WATCH FOR:** The fast student who is fast because they skip the predictions and the questions. Speed through
> the "Do" steps proves nothing. Look at their written answers before you hand out an extension.

## The student who is struggling

Find out which kind of struggle it is. They need different help.

| The trouble | What it looks like | What helps |
|---|---|---|
| **Typing** | Everything takes three times as long; many errors that are slips, not misunderstandings | Pair with a typist, swapping roles. Teach <kbd>↑</kbd> to recall and repair a line. Allow the editor and paste for long listings: a card image with the starting files (Topic 3) |
| **Reading** | Can do it when shown, not from the sheet | Read the step aloud together. The labs' steps are short on purpose. For the manual's reading, set less: each lab's notes in Topic 5 say which sections are essential |
| **Precision** | "It doesn't work" twenty times a lesson: capitals, spaces, the wrong quote | The read-it-aloud habit: "what did you type, exactly? What did it say, exactly?" Card 1 beside the keyboard |
| **The idea itself** | Types correctly, can't predict | Go back to the physical model: cards for the stack (Lab 16), a drawn tree for files (Lab 7), students acting the registers (Lab 24) |
| **Confidence** | Won't press <kbd>RETURN</kbd> without checking with you | Say, and show, that nothing typed can break it: break things at the projector, on purpose, and reset. The manual's section 6.1 says the same |

For a student well behind, cut the course rather than rush it. Labs 1, 2, 3, 5, 7, 20 and 21, and Project 1, are
a complete and honourable course on their own.

## Younger students (12 and 13)

* **HyBASIC**, and only HyBASIC. It reads like English, and its error messages are words.
* **Shorter sessions at the keyboard**: a lab split over two lessons, with the questions as the second.
* **The emulator**, with the Vera X on: sound and pictures (Labs 12 and 13) are the hooks at this age.
* **In a build**: through-hole parts only, in threes, with an adult for every six, and the adult doing Step 1,
  the alcohol, and the first power-up. They solder as well as sixteen-year-olds after the first evening. They
  need more help *reading* the parts.
* **Leave out** Chapter 7's detail, namespaces (the second half of Chapter 9), and Part Five, unless a student
  asks for them. Some will.

## Older students and adults

* **Skip nothing in Chapters 7 and 9.** These are the chapters that an older student finds genuinely new: most
  have programmed, and few have ever seen how an operating system starts or what a namespace is.
* **Two languages, at least**, and Lab 23.
* **Part Five for everyone.**
* **The design questions.** Why sixteen tasks? Why are devices files? What would you change? Topic 2's Shape F
  has more.

## Students who can't solder

Some students can't, or shouldn't: a tremor, a visual impairment, a respiratory condition, or simply a refusal
that you've decided to respect. A build has plenty of real work away from the iron:

* **Reader and checker**: reading each step and each part's value aloud, and ticking the Companion. It's the job
  that decides whether the board works (Topic 6).
* **Inspector**: the magnifier, and the final say on whether a joint passes.
* **Kit manager**: sorting, counting, identifying parts, and handing over the right one.
* **Tester**: the multimeter in the power test, and the terminal at switch-on.
* **Fault diarist**: the keeper of what's been tried.

Rotate everyone through these jobs, so that they're roles in a team and not a consolation.

## Seeing, hearing and moving

* **Colour.** Resistor values are in coloured bands, and about one boy in twelve can't tell some of them apart.
  The manual's tip is the answer for everyone: measure it with the multimeter. Never make reading the bands by
  eye the only way.
* **The terminal's colours** can be changed in most terminal programs; the Hydra's own text needs no colour to
  be understood. Lab 10's colour questions can be answered from the escape sequence alone.
* **Small text.** The web emulator's console scales with the browser's zoom. On a real Hydra, the terminal
  program's font is yours to choose.
* **Sound.** Labs 12 and 22 and Project 3 have a visible side: the score is text, and a tune that's wrong can be
  seen to have bars of the wrong length. A deaf student can do all of the notation, and check it by counting.
* **Fine movement.** Soldering is covered above. At the keyboard, sticky keys (the operating system's own
  accessibility setting) make <kbd>CTRL</kbd>-<kbd>]</kbd> and the other two-key combinations possible with one
  hand.

## Language

The manual is written in short sentences and plain words on purpose, and it defines every technical term where
it first appears (in bold) and again in Appendix J. For students working in a second language:

* **The glossary first.** Appendix J is a word list as much as a glossary. A page of it for homework, before each
  part, helps more than anything.
* **The Hydra's own words are few.** About fifty commands, each a short English word or an abbreviation. A card
  of the twenty on Card 2, with translations written beside them by the student, is enough for months.
* **Error messages are the hard part**, because they're terse. Lab 3's table of mistakes, and the "When it goes
  wrong" tables on Cards 4 to 6, turn them into a list that can be learned.

## Girls, and everyone else who's been told this isn't for them

A few habits make a room where everyone stays, and none of them cost anything:

* **Pairs, by your choice, not theirs**, and changed often.
* **Rotate the iron and the keyboard on a timer**, so that nobody is ever "the one who watches".
* **Projects with a choice of subject.** A quiz about anything; a tune of their own; a picture of their own. The
  starting listings were chosen so that the subject is open.
* **Praise the method, not the talent.** "You found that by changing one thing at a time" tells a student what
  to do again. "You're a natural" tells the others that they aren't.
