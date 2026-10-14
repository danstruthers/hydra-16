# Dingus Quest Zero's theme and denizens

The design of what a dungeon is about and who lives in it: what the user asked for (2026-10-14), and how the
engine can do it.  The dungeon's shape is DUNGEON.md's; the rules questions go to the referee in rs.h's terms.

## What was asked

- **A theme.**  Each dungeon has a theme, chosen at random as the game starts, aligned with the puzzle the
  character must solve (the pieces to find, the mission: to escape, or to become strong enough to rule the place).
  The theme helps decide how the dungeon grows, what kinds of denizens it has, and their attitudes.
- **Not every denizen is hostile.**  Some can become allies: still run by the denizens' task, but working, perhaps,
  for the player.
- **Influence.**  Rhetoric and the other skills of persuasion and presence can change the attitude of a denizen
  that can be swayed that way; Animalcraft (HarnMaster's skill of handling animals) can do the same with animals.

## How

### The theme

A theme is data (a file of the game's, one of several, picked by the game's seed), not code.  It says:

- **the puzzle:** its pieces (what each is, where it may lie: a stratum's style or depth, a faction's keeping), what
  bringing them together does, and the mission's endings (the way out opened, the dungeon ruled);
- **the dungeon's growth:** its strata, from the top down: each one's style, age and size, as ranges to pick from
  (a drowned temple: built strata over flooded caves; a dwarf-hold: halls and mines, its deepest abandoned), the
  names, the words for rooms and their light, the features it favours (shrines, cisterns, forges, ossuaries), and
  where the pieces may lie;
- **its factions:** the peoples and creatures of the place, each with its kinds (the referee's creatures, by key), the
  strata where they live, how many, and their attitude to the character as they first meet it and to each other;
  what can sway them (persuasion, presence, a bribe, a shared enemy, a piece of the puzzle) and what can't (the mindless
  dead, a beast that's only hungry);
- **its animals:** the beasts, which Animalcraft can calm, befriend or drive off, and none of the persuasive skills
  can.

### Attitudes

Every denizen has an **attitude** to the character, on one scale whatever the ruleset: **hostile** (attacks on
sight), **unfriendly** (threatens, keeps its distance, attacks if pressed), **wary** (watches, won't help), **neutral**,
**friendly** (talks, trades, tells what it knows), **ally** (comes with the character, fights beside it, takes its
requests), **devoted** (follows it into anything).  A faction has an attitude as a whole, which its members start
from; and attitudes between factions (a war, an alliance), so that what the character does to one changes what
others think of it.

An attitude moves by what happens: an **influence** (below), a gift, a rescue, harm done to it or to its own, a piece
of the puzzle shown or taken, the character's strength as it sees it (rs.h's power, 0-100: the strong are feared,
the weak despised or pitied, as the faction is).

### Influence

The player tries to sway a denizen it can talk to (a shared tongue: the referee knows the character's languages) or,
for a beast, one it can reach: **persuade**, **intimidate**, **deceive**, **entertain**, **bargain**; for animals, **calm**
and **befriend**.  Each is a purpose the referee tests (rs.h: P_PERSUADE, P_INTIMIDATE, P_ANIMALS ...; HarnMaster's
Rhetoric, Oratory, Intrigue, Animalcraft, Singing ...), as a **contest** where the denizen resists (its Will, its
faction's stubbornness), the circumstances as steps (its attitude now, the theme's say on what moves it, gifts,
threats seen, its fear of the character): a success moves the attitude a step its way (a critical one two), a
failure doesn't, a critical failure a step back (an insult, a bite).  Some attitudes can't move past a limit (a
guard's duty, a hunter's hunger) without more than words.

### Allies

An ally is still the **denizens' task's**: it moves, fights and acts on its own turn by its own nature, but its goal is
the character's; it follows the character, takes requests ("guard the door", "wait here", "follow me", "attack the
ghoul"), and keeps them as far as its attitude and courage go (the referee's morale).  An ally can be lost: harm it,
leave it, ask too much, and its attitude falls.  The party, designed for from the start, is the character and its
allies.

### In the tasks

- **The world** keeps each denizen's place and state, carries out its intents as it does the character's, and tells
  what happened as events.
- **The denizens' task** decides what each denizen means to do, by its kind, its attitude, its faction's goals and
  what it senses (each denizen sees and hears through its own senses, as the character does); allies by the
  character's requests.
- **The referee** answers the tests and contests (influence, morale, combat), and keeps each creature's sheet.
- **The UI** words it all, and parses the player's influence and requests ("persuade the goblin", "tell Grub to wait").
