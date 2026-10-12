# Topic 7. Assessment

Three kinds of work come out of a Hydra course: lab sheets, projects, and (perhaps) a build. Each wants a
different kind of marking. This topic has a way to do each, with rubrics you can copy; Topic 16 has a bank of quiz
questions with answers.

## What to assess, and what not to

**Assess understanding, not typing.** A lab's "Do" steps can be completed by a student who understands nothing:
the table fills in itself. The marks are in the predictions, the written questions and the "check yourself" list.

**Assess the process as well as the product.** A project that works, with an empty logbook, has taught less than
one that half works, with three pages on why. The fault diary of a failed build is worth more than a Hydra that
happened to start.

**Don't assess speed.** The fast typists aren't the deep thinkers, and the labs' times are guides.

## Lab sheets

Marking twenty-six labs for thirty students in full is not possible, and not needed. Do this instead:

* **Every lab: a glance**, in the lesson, while you circulate. Are the predictions filled in, in pencil, before
  the answers? Tick the sheet. This takes seconds, and it's what keeps the habit alive.
* **Every lab: self-checking.** Students tick their own "check yourself" lists. Collect the unticked boxes, on a
  show of hands: they tell you what to reteach.
* **A sample, marked properly**: one question from each lab, chosen by you after the lesson, for every student; or
  the whole of one lab in four, in rotation. Part Three has the answers.

A four-level scale is enough for a written answer:

| Level | The answer |
|---|---|
| 3 | Is right, and says *why*, in the student's own words |
| 2 | Is right, but only states the fact, or copies the manual |
| 1 | Has the right idea, with a mistake; or is right about what happened, and wrong about the reason |
| 0 | Is missing, or shows no understanding |

The difference between 2 and 3 is the one that matters. "`/ram` loses files at a reset" is a 2. "`/ram` is made of
memory, and memory is wiped when the Hydra starts, so anything only there is gone" is a 3.

## The Skills Passport

The Companion's last pages list 44 skills, each with a place for a signature. Use it as a practical assessment
that runs all course long:

* A skill is signed when the student **does it in front of someone**, without notes: you, or a student who
  already holds that signature.
* Peer signing works, and it's how you find time. Spot-check one in five.
* The "Understanding" skills are signed after a spoken explanation of a minute or so. These are the hardest to
  earn, and the best evidence you'll get.

## Projects

A project is assessed on four things. Share the rubric with the students before they start.

| | 4 | 3 | 2 | 1 |
|---|---|---|---|---|
| **It works** | Does everything it set out to; copes with bad input | Does what it set out to, with ordinary input | Runs, with faults | Doesn't run, or is the starting listing unchanged |
| **It's the student's own** | Four or more changes from the starting point, one of them the student's own idea | The changes asked for | One or two small changes | None |
| **It can be read** | Clear names, comments that say *why*, tidy layout; built from small pieces, each tested | Comments and layout good enough for a classmate to follow | Hard to follow | No comments; one lump |
| **It can be explained** | The student explains any line asked about, and what they'd do differently | Explains how it works overall | Explains what it does, not how | Can't explain it |

"It can be explained" is assessed by talking: three minutes at the student's own screen. Ask two questions: *"what
does this line do?"*, pointing at one that isn't trivial; and *"what was the hardest bug, and how did you find
it?"* The second question, asked every time, teaches students to notice their own debugging.

**The showcase.** End every course with one. Each student has three minutes to show one thing running to people
who haven't seen it: another class, parents, the head. It isn't marked. It's what makes them finish.

## The build

A build is group work on a single object, so assess the individual's part in it through the records:

| | Evidence | Looking for |
|---|---|---|
| **Safe working** | Your own observation; the signed rules | Glasses on, iron in its stand, without being told. This is pass or fail, and failing means off the bench until it's put right |
| **Workmanship** | The student's own practice board; their signed rows in the Companion's Unit 2 | Joints to the manual's standard; parts the right way round; faults found and repaired |
| **Method** | The fault diary; the student's logbook | One change at a time; results written down; a reason given for each check |
| **Understanding** | A spoken question at the bench | "What is this part for?" "What would happen if it were in backwards?" "What did that test rule out?" |

Never mark a student down because the group's board didn't start. Mark what they did about it.

## A written test

If you must have one, Topic 16 has sixty questions, in six sets of ten, with answers. They're short-answer
questions, not multiple choice: a student who can write "the kernel gives each task a turn, two hundred times a
second" has shown something that choosing (c) doesn't.

Better than a test, if your timetable allows: **a practical.** Twenty minutes at a Hydra, with the manual
allowed, and five tasks on a card. For example, after Shape B:

1. Make a directory `/sd/0/test`, and in it a file with your name in it.
2. How many lines in `/sd/0/sample/as/tick.s` contain the word `SLEEP`? Write the command and the answer.
3. Start `sleep 300` in the background; find its task number; stop it.
4. Write a three-line script that prints the date, then how many programs are in `/rom/bin`.
5. Write a HyBASIC program that prints the 9 times table.

Using the manual is the point, not a concession: looking things up *is* the skill.

## Reporting

For a report home, three sentences cover it: what the student can now *do* (from the Passport), what they *made*
(the project, by name), and how they *work* (from the logbook and the fault diary). Parents understand "built a
working computer, wrote a quiz game in BASIC, and found a fault by methodical testing" better than any grade.
