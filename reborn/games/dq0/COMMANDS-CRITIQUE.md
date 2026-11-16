# The command processor: a critique, and how I'd build it again

A review of how DQ0 turns a typed line into an effect on something named in the world. It covers the current
design, where it breaks, why, and a design that would make every verb behave the same way on every kind of thing.
It ends with a migration path that doesn't need a rewrite.

Sources: commands.ebnf / .verbs / .words / .actions / .aliases / .corpus, dq0c (cmdmatch.s, cmdmean.s and its host
reference sim/tools/commandwords.js), main.c's `s_command` and its overflow handlers (chores, words.c's `o_more`,
`s_rarer`, `s_missiles`, theme.c's `o_puzzle`), resolve.c, choose.c, way.c, the world's item store (dq0w/things.c),
the denizens' `meant` (dq0d.c), and DECISIONS.md / HOUSE.md / DUNGEON.md. The behaviour quoted is from two games I
played in the emulator (seed 3, a hunter, the kitchen at the start). The command lists are at the end.

---

## 1. What exists today

```
line ─► alias.c / names.c / pace.c / parts.c   (each reads the raw text first)
     ─► dq0c: cmdmatch.s   grammar says yes/no (commands.ebnf)        ─┐  two separate
             cmdmean.s    bag-of-words scan + "old parser's rules"   ─┘  readings of one line
     ─► struct cmd { action, aux, dir, prep, how, ..., obj, other }
            obj/other = struct cthing { kind (OB_* fixture) | word[16] }
     ─► choose()/answer()  resolve.c  (items & denizens in sight: closest; "it"; unsaid)
     ─► puzzle()           theme.c intercepts read/combine/use/put
     ─► switch (action) in s_command
            ├─ portals ......... find() choose.c (doors: ahead first) → I_PORTAL(PA_*)
            ├─ items ........... things(IO_*, word) → world's things.c pick() (most to hand)
            ├─ denizens ........ deal(SW_*) word → dq0d meant() (nearest of that name)
            ├─ place parts ..... place_go / look_way / search_go, each its own logic
            └─ default → chores() → more() → rarer() → unknown(): "You can't X in this game yet."
```

A lot here is right, and worth keeping:

- **An action catalogue with roles** (VB_* + object/instrument/target …), separate from the words that say it.
- **Verbs as data** (commands.verbs: the shape → action, the unsaid default, aux, skill, task).
- **One intent structure** (I_*) shared by the player and the denizens' minds.
- **Scope is perception.** You can only name what the character perceives or remembers.
- **Questions only when it matters**, with numbered choices answered by number, way, selector or name.
- **"It", the target, unsaid defaults, walking over to things out of reach, long tasks, first words before the answer.**

These are the features of a serious IF parser. The problem isn't the ideas. They are implemented four or five
times over, by different parts, with different rules, and the parts don't share their results.

---

## 2. What the player sees

From the two games. Every line is the game's actual answer.

| Typed | Answer | What's wrong |
|---|---|---|
| `look at the pot` | "The iron pot is beside you. It's closed." | ✔ (items work well) |
| `look at the stairs` | turns to face NW, "You peer ahead. You can't see past the door ahead" | looked at a door, not the stairs |
| `look at me` | the room's description | object dropped |
| `examine the stairs` | the room's description | object dropped |
| `climb the pot` / `climb the well` / `climb the backpack` / `climb the door` | "The way out is shut fast…" | climbs the stairs; the object is ignored |
| `sit on the pot` / `sit on the backpack` | "You sit down." | object ignored |
| `light the door` | takes off the pack, gets out a new torch, lights it | the door is replaced by something else, silently |
| `smell the torch` | sniffs the air | object ignored |
| `smell the door` | "Which door: 1. … or 2. …?" | asks which door, then sniffs the air anyway |
| `put the torch on the door` | "The torch won't hang from a belt." | a nonsense refusal |
| `take the door` / `take the stairs` / `take the wall` | "Take what?" | the noun was understood but read as missing |
| `take the well` | "The well is fixed where it stands." | ✔, but only because the well is an *item* |
| `open the stairs` | "There's no door that way." | |
| `kick the door` | (a blow at the door) | |
| `kick the pot` / `kick the well` / `kick the backpack` | "There's nothing like that here to fight." | |
| `kick the stairs` / `kick the wall` | "There's nothing beside you to fight." | one verb, three different messages |
| `push the door` / `touch the door` | "You can't push in this game yet." | |
| `push the door behind me on my right` | "I don't understand that." | the grammar refuses it (see §3.1) |
| `use the dagger on the door` | "Use what, and on what? (use the key on the way out …)" | both were said |
| `search the door behind me on my right` | turns to face SW, "There's no door that way." | `kick` with the same words had just found that door |

Items held, carried or lying about get rich, consistent treatment, because they all go through one pipeline (the
world's item store). Everything else depends on which verb you used. There are three common failures:

1. **The object is silently dropped.** The verb does its no-object version, so the player believes the noun was understood.
2. **The object is silently swapped for something else.** `light the door` lights a torch.
3. **The refusal is accidental.** You get the message of whatever path the command happened to fall into.

---

## 3. Root causes

### 3.1 Two parsers read every line, and neither one's structure is used

`cmdmatch.s` matches the line against commands.ebnf and returns **only yes or no** plus its tokens. The meaning is
built separately by `cmdmean.s` (`scan()` in commandwords.js). It walks the words left to right, putting each into
a slot by its word class, then applies a list of patch-up rules ("put … away → stow", "turn … on → light",
"get … from with no other → doff", "with: swap word and word2" …). The grammar's rules know that in
`<put> ::= PUT <thing> <put-prep> <object>` the second noun phrase is the destination. That knowledge is thrown
away, and the scan guesses it again.

Consequences:

- **Two sources of truth that drift apart.** In the grammar, a noun phrase with a location (`<thing>`, which allows
  "the door behind me on my right") is allowed after some verbs, while others take only `<object>`. So
  `push the door` gets "can't push yet", but `push the door behind me on my right` gets "I don't understand that".
  Likewise `climb`, `light`, `put … on`, `use … on`: each verb decides for itself whether the same noun phrase is
  legal.
- **A third and fourth reading in the UI.** main.c and its helpers read the raw text again with `strstr(cl, …)`:
  - `" behind "`, `" in "`, `" under "` for hide;
  - `"hand"`, then a scan for "left hand", "both hands" …;
  - `"kneel"` to tell kneel from crouch;
  - `"sharpen"`, `"hone"`, `"whet"` to tell sharpening from filling;
  - names.c, alias.c, pace.c and parts.c each read the line before the parser does.

  `s_hand_said` exists because the meaning scan read "left" as a direction and "both" as a count, so the UI undoes
  the parser's work by re-reading the text.
- **The corpus tests the wrong layer.** commands.corpus checks that a line gives the right action. It never checks
  what the game *does* with it. That is why ~41 of the 145 actions have no handler anywhere (push, pull, touch,
  knock, taste, wake, swim, say, charge, engage, grapple, cast, pray …) while the grammar and corpus happily
  accept them.

### 3.2 There is no single notion of "a thing that was named"

`struct cthing` is *either* a fixture class (`kind = OB_DOOR, OB_STAIRS, OB_WALL …`) *or* a word (`word = "torch"`).
It is never a reference to a particular object.

- A word in `[things]` sets `kind` and is otherwise discarded. So `take the door` arrives as "take, no word" →
  "Take what?".
- Two noun phrases can collapse into one slot. `use the key on the door` parses to
  `obj.kind = OB_DOOR, obj.word = "key", other = (empty)`. `attack the door with the axe` gives
  `obj.kind = OB_DOOR, obj.word = "axe"`. In each case the instrument and the target share one structure.
- The things a player can name live in at least seven id spaces, each with its own record:

  | Kind of thing | How it's identified |
  |---|---|
  | Items | the store's number; the cache is renumbered as regions page in |
  | Portals | a square |
  | Openings (stairs, ladders, shafts, windows) | a relative way plus a square |
  | Traps | a relative way, no square |
  | Room zones | a relative way |
  | Denizens | an actor slot, or a name string |
  | Rooms | rx, ry |
  | Parts of a portal | a word flagged `PART` |

  The senses' `struct sight` carries them as eight separate arrays.
- **What counts as a "fixture" is an accident of implementation.** A well, a sconce, an altar are *items* with
  `KF_FIXED`, so they get everything items get (look, examine, look in, take → "fixed where it stands"). Stairs, a
  wall, a ledge, a pillar are `OB_*` place parts, so they get almost nothing. The player can't predict which is which,
  and shouldn't need to.

### 3.3 Things are resolved four times, by four rule sets, and the results are thrown away

| Chooser | Covers | Rule |
|---|---|---|
| resolve.c `gather`/`choose_one` | items and denizens in sight | closest; ordinal left to right |
| choose.c `pick` | portals and openings | the one ahead, else ask |
| dq0w/things.c `pick` | items | most to hand: held > worn > packed > reach > far |
| dq0d.c `meant` | denizens | nearest of that name within 4 squares or seen |

DUNGEON.md (2026-11-04) records the decision that "the choosers stay two … not merged".

What makes this a correctness problem rather than just duplication: **resolve.c's answer doesn't reach the part
that does the work.** It writes the chosen square into `cmd.obj.x/y`, but only throws (`main.c` `s_missiles`) and
walks to a thing (way.c) read it. `take`, `drop`, `put`, `open the chest`, `light`, `fill`, `hide`, and every
`deal()` to a denizen send **the bare word** (`things(IO_*, cmd.obj.word)`, `strcpy(s->word, cmd.obj.word)`), and the
world or the denizens' task chooses again by its own rule.

- If the player answers "Which rat? 2", the denizens' task then picks the nearest rat anyway.
- The torch playtest's bugs on 2026-11-06 are this problem:
  - `drop the spent torch` dropped the lit one;
  - "You reach for the torch" when it was already in hand.

  The fixes so far add more selector words (SL_SPENT, SL_FULL, SL_NEW …) for the world's chooser to interpret, rather
  than sending the thing that was chosen.

### 3.4 Dispatch is by verb, with each verb guessing the kind of thing on its own

`s_command` switches on the action. Each case then works out what kind of thing it got, in its own way:

- `if (!cmd.obj.kind && more(action))`: an openable item, else a portal;
- `may_den() && deal(SW_LOOK|SW_QUIET) != SR_NONE`: examine asks the denizens' task first;
- `fight_target() == FT_THING`: attack becomes a portal break;
- `WAY_OBJ >> kind`: look_way decides whether a kind is something you can turn toward.

There's no place in the code that answers "what does verb V do to a thing of class C?" A combination nobody wrote
code for falls through to the verb's *no-object* version, which is where every silent drop in §2 comes from:

- `VB_CLIMB` always intends `I_CLIMB` up or down;
- `VB_SIT` always sits;
- `VB_SMELL` with no way given sniffs the air;
- `VB_LIGHT` with no item word lights what's in hand, or gets a new torch.

The dispatch is also spread across many functions: `s_command`, `chores`, words.c's `o_more`, `s_rarer`,
`s_missiles`, `limbs_cmd`, `look_way`, `search_go`, `place_go`, theme.c's `o_puzzle`. Which function a case lands in
mostly depends on which overlay had room ("part 2 has no room", "overlay 15 has no room"), not on what kind of
case it is.

### 3.5 Refusals come from whatever path the command fell into

Nobody designed the refusal messages. Each comes from whichever path a command fell into: "Take what?",
"There's nothing like that here to fight.", "There's nothing beside you to fight.", "There's no door that way.",
"The torch won't hang from a belt.", "I don't understand that.", "You can't X in this game yet.". A player learns
the game's rules from its refusals. If they're inconsistent, the world feels arbitrary even where the simulation
underneath is careful.

### 3.6 What a thing is gets decided by matching strings

Kinds of things are recognised by substrings of their names, scattered through the handlers:

- `strstr(w, "rope") || strstr(w, "grapple")`, `"arrow"/"bolt"/"stone"`;
- `"key"/"pick"` decides whether something can open a lock;
- `strcmp(word, "bar")` turns "take the bar off" into unbarring;
- `"bars"` with no window means a portcullis;
- `"antidote"/"remedy"`, `"waterskin"`, `"money"/"coins"`.

Other places where words are doing the work of data:

- Names cross task boundaries as 16-byte words.
- A plural is a trailing "s".
- Adjectives are checked against a fixed list, and only for things seen, not things carried (HOUSE.md admits this).

Every new item kind risks colliding with one of these string tests, for example a "keystone", a "stone key" or
"rope ladder".

---

## 4. The design, if built from scratch

Five rules. Each one replaces a set of special cases.

1. **Parse once.** The grammar produces the roles. Nothing after the parser reads the text again.
2. **Name once.** Everything nameable is an entry in one scope table and is referred to by a handle.
3. **Resolve once.** One resolver produces handles, and the handles, not words, go to the world and the minds.
4. **Dispatch on (action, class)** through a table. A cell nobody filled in gives a designed refusal, never the verb's no-object version.
5. **Refusals are data**, keyed by (action, reason), so every refusal is worded the same way.

### 4.1 The scope table: one list of everything nameable

At each command the senses build one list (it replaces `struct sight`'s eight arrays for naming purposes; the
descriptive arrays can stay for the look's words):

```c
struct ent {                 /* ~12 bytes; 32 entries ≈ 384 bytes */
    unsigned char cls;       /* EC_ITEM, EC_PORTAL, EC_FEATURE, EC_DENIZEN, EC_SELF, EC_PART, EC_ROOM */
    unsigned char noun;      /* noun id (the dictionary's), not a string */
    unsigned char adj;       /* adjective bits: colour/material/size, and the kind's own ("black", "iron", "giant") */
    unsigned char state;     /* ST_OPEN, ST_LOCKED, ST_LIT, ST_SPENT, ST_EMPTY, ST_DOWN, ST_HELD … */
    unsigned char aff;       /* affordances: what can be done to it (below) */
    unsigned char where;     /* held L/R, worn, in-pack(level), in reach, ahead, in sight, remembered */
    unsigned char rel, dist; /* bearing as it faces, squares */
    unsigned      ref;       /* the owner's id: item number | portal square index | actor slot | feature index */
    unsigned char parent, comp;  /* the scope entry it's in or on, and which of that holder's compartments (§4.9) */
};
```

- **Every noun gets an entry.** Stairs, a wall, the floor, a ledge, a pillar, a well, a sconce, a portcullis's bars,
  the character ("me", EC_SELF, with body parts as EC_PART children), the room. Whether something is an *item* or a
  *feature* becomes the owner task's business. The parser and resolver never need to know.
- **Affordances** are a byte or two of flags taken from the kind (catalogue) or the portal or feature type:
  `PORTABLE, OPENABLE, LOCKABLE, CONTAINER, SURFACE, CLIMBABLE, SITTABLE, LIGHTABLE, EDIBLE, DRINKABLE, READABLE,
  ANIMATE, BREAKABLE, FIXED`.
- **Nouns and adjectives are numbers**, from one dictionary built at make time out of the catalogue's kinds, the
  portal and feature types, the denizen kinds, and synonyms. Plurals are dictionary entries, not a trailing-s rule.
  This also removes the strings dq0c needs from the world at startup (`cm_world`'s word lists become noun ids).
- **The designators being built now are names for scope entries.** `(A) a gargun` and `(c) 3 torches` each name one
  entry (one stack). See §4.8: designators are the reason to build the scope table, not a separate feature.

### 4.2 The parser: one pass, with the roles recorded as it matches

Keep the hand-written 65C02 matcher. It's fast, and the 720,000-cycle budget is real. Change what it produces:

- **Label the roles in the grammar** and have the matcher record what each label matched:

  ```
  <put>   ::= PUT <np:obj> <put-prep:prep> <np:dest> | PUT <np:obj> <hand:hand>
  <unlock>::= UNLOCK <np:obj> [ <USING> <np:instr> ]
  ```

  The matcher already knows where each rule ends. Recording a (role, first token, last token) entry as it accepts a
  labelled rule is a small stack, and it replaces `scan()` and every patch-up rule.
- **One noun-phrase rule, used everywhere:**
  `NP ::= [det|count|selector] {adj} noun [locative-qualifier]`, where the qualifier is
  `on my left | behind me | in my pack | on the table | from the floor`.

  A verb chooses its *frames* (`V NP`, `V NP prep NP`, `V prep NP`, `V DIR`, `V`), never its own kind of noun phrase.
  The `<object>`/`<thing>` split goes away, and `push the door behind me on my right` parses like any other noun
  phrase.
- **Verb frames as data** (an Inform-style grammar line per frame) in commands.verbs, compiled to the same tables:

  ```
  put    NP:obj ON|IN NP:dest  → VB_PUT     wants obj=CARRIED  dest=SURFACE|CONTAINER
  put    NP:obj DOWN           → VB_DROP
  climb  NP:obj                → VB_CLIMB   wants obj=CLIMBABLE
  climb  DIR                   → VB_CLIMB
  ```

  `wants` is a hint to the resolver (§4.3), not a requirement.
- **The output** is `struct cmd` carrying up to three noun-phrase descriptors (noun id, adjective bits, selector,
  count, way it lies, qualifier) and, after resolving, three **handles**. Hands ("in my left hand"), manner and
  aspects are roles in the grammar, so `s_hand_said` and the other `strstr(cl, …)` reads disappear.

### 4.3 One resolver

One function takes a noun-phrase descriptor and the scope table, and returns a handle, a list (for "all", a count,
"both"), or a question:

1. **Filter** by noun, adjectives, state selectors (lit/spent/open/empty/new) and the location qualifier.
2. **Rank** by one salience order: *held > worn > in pack (shallower first) > in reach > ahead > in sight >
   remembered*, then adjusted by the frame's `wants`:
   - `take` prefers things not carried;
   - `drop`, `put`, `throw` prefer things carried;
   - `attack` prefers ANIMATE;
   - `climb` prefers CLIMBABLE.

   Ordinals ("the second") count in that order; "closest", "largest" and "lead" re-sort.
3. **Decide:** one clear winner → the handle. A tie the `wants` can't break → the numbered question, as now (the
   question code from choose.c and resolve.c merges into one). None → a designed refusal ("You see no rope here.",
   "You haven't seen a door here.").
4. **Remember:** "it", "them", "him"/"her", the target and the last thing named are handles too. They are checked
   against the scope table each command; one that has gone out of scope gets a refusal, not a guess.

**The handle is what gets sent:** an item number in `ireq`, a portal square in `I_PORTAL`, an actor slot in `sway`.
The world's `pick` and the denizens' `meant` stop being choosers. They're kept only for the minds' own choices
("the nearest thing I want"), which aren't made from words anyway. That removes the double-choice bugs as a class.

### 4.4 Dispatch: the action × class table, and a fixed set of phases

Each command goes through the same phases, as Inform's before/check/carry out/report rules do, but as tables
rather than rulebooks.

```
for each action:  flags      NEEDS_REACH | NEEDS_HELD | NEEDS_LIGHT | NEEDS_HANDS | TAKES_TIME | IS_SOCIAL ...
                  requires   affordance mask per role   (VB_CLIMB: obj needs CLIMBABLE)
                  handler[cls] index (sparse: one byte per class; 0 = the action's default)
```

1. **Scope.** Is the handle still perceived? Generic.
2. **Affordance.** Does the role's entity have the required flag? If not → refusal (VB_CLIMB, NOT_CLIMBABLE, "the pot")
   → "The iron pot isn't something you can climb."
3. **Preconditions, done for you** (generic, from the flags): walk over if out of reach; take it in hand if
   NEEDS_HELD; open the container if the thing is inside a closed one; stand up first if seated. Each is an intent
   chained ahead of the action. These already exist piecemeal (reach_go, hold_first, the reach's steps); this
   makes them one mechanism that every action gets.
4. **Carry out.** `handler[action][cls]` → the owner task's request (IO_* / I_PORTAL / SW_* / I_FEATURE).
   **A missing cell goes to the action's default**, which is a refusal ("You can't push the door. — not yet in
   this game") and never the action's no-object version.
5. **Report.** The owner's words, or a templated success message keyed by (action, class).

The table is small: ~145 actions × 8 classes at one byte each is ~1.2K, in the catalogue's bank with the handlers.
It also becomes the **coverage report**. Every cell is *handled*, *refused by design*, or *missing*, and a build
step can print the missing cells.

The overlay scheme is unaffected. Handlers still live in whatever part or overlay has room. The table gives each
handler's stub index, so where a case lives no longer decides how it behaves.

### 4.5 Refusals and messages as data

One table, `(action, reason) → template`, with `%n` for the noun phrase as the player would say it:

| reason | template |
|---|---|
| NOT_IN_SCOPE | "You see no %n here." |
| NOT_AFFORD | "%N isn't something you can %v." |
| NOT_YET | "You can't %v %n in this game yet." |
| FIXED | "%N is fixed where it stands." |
| ALREADY | "%N is %s already." |
| NEED_INSTR | "You have nothing to %v %n with." |

A row per action can override a template where the wording should differ ("You can't climb that." versus "There's
nothing to hold on to."). That's tens of bytes per action. The game's prose style is preserved, and it's applied
everywhere.

### 4.6 Tests that check behaviour, not just the action parsed

- **The corpus says what happens:** `line · scene → action + handles → outcome class` (done / refused(reason) /
  asked). "Scene" is a small fixture room (a door, stairs, a well, a pot on the floor, a carried sack, a rat).
- **A sweep of every action against every class:** in that one scene, type every verb at every class and assert
  that the answer is either the handler's or the refusal for that cell. That is the probe I ran for this review
  (§7) turned into a test, run at the turns pace. It's the test that would have caught every row of §2.
- Keep the hub writers' check and the soak. The sweep goes beside them.

### 4.7 Does it fit the machine?

| Cost | Size |
|---|---|
| Scope table | ~400 bytes in the senses' or UI bank (replaces what `sight` holds for naming, and resolve.c's `cn/cx/cy/ch/cs/cd` copies) |
| Role capture in the matcher | a stack of ~8 entries × 3 bytes, plus a little matcher code |
| Dispatch and refusal tables | ~1.5–2K, in a bank |

What it removes:

- `scan()`'s patch-up rules and the corresponding cmdmean.s code;
- three choosers' worth of naming logic;
- the `strstr` string matching;
- most of the "which kind did I get" branching in `s_command`;
- the second resolution in the world (`pick`) and in the denizens' task (`meant`), which DECISIONS.md measured at
  over a second for a denizen check on examine.

I'd expect a net reduction in code and in cycles per command. The first-words promise (D17) gets easier too. Once
the UI has a handle it knows exactly which thing it's reaching for, so "You reach for the torch" no longer has to
predict the world's choice.

### 4.8 Designators: the player-facing side of handles

The design being built (DECISIONS.md, 2026-11-06), loosely NetHack's:

- **Lowercase, one or two letters:** stacks carried or worn.
- **Uppercase, one or two letters:** stacks and denizens seen. Denizens are lettered first. A letter is kept while
  the thing is remembered.
- **A count before a letter:** `take 3A`, with a warning if the stack has fewer than three.
- **Shown in brackets:** in the look, in a list of what's in sight, and in a one-item-per-line inventory.
- **Things are stored in stacks.**

In this design, a designator **is** a handle. Typing `drop c` is the same as typing `drop the spent torch` and having
the resolver answer with stack *c*. Everything after resolving is the same pipeline: affordance check, generic
preconditions, the (action × class) handler, the refusal table. What this means for the work under way:

1. **What gets sent must be the stack's identity, never its name.** If `drop c` turns into
   `things(IO_DROP, "torch")`, the world's `pick` chooses again by "most to hand", and a player who chose *c* gets the
   lit torch: the torch playtest bug, now with a letter in front of it. The UI has to send the item's **permanent id**
   (`struct item`'s `id`, which is never reused), not its cache index, because the 744-record cache renumbers as
   regions page in. The same goes for denizens: the actor slot or denizen number goes in `sway`, not the word for
   `meant()` to look up again. This is step 1 of §5, and the designators need it whatever else is done.
2. **Lettering needs one combined, ordered list.** Uppercase letters go to *denizens first, then things*. That's one
   list over two of today's sources (`sight.den[]`, `sight.item[]`), which is the first part of the scope table
   (§4.1). If it's built as one table with `cls` and `ref`, steps 4 and 5 are already half done. If it's built as a
   letter column on each of the two arrays, it becomes a fifth chooser.
3. **The handle is (stack, count).** "3A", "half of A", "all of c" and "both torches" all become the same resolver
   output: a stack handle plus a count (a number, CT_ALL, or CT_HALF). The world splits the stack (KF_COUNT already
   splits and joins lots). "Only 2 in the stack" is one refusal row, the same for typed counts and noun-phrase counts.
4. **Questions should list designators.** "Which torch? (c) a lit torch, (d) a spent torch, (B) a torch on the floor"
   replaces "1. … 2. …". The answer is a designator, which is already a valid noun phrase, so the separate question
   paths in choose.c and resolve.c (`answered()`) shrink to "answer a pending question with a handle". Numbers then
   mean only counts, which removes the number-or-choice ambiguity in "take 2".
5. **Letters are mostly taken already.** Of the single lowercase letters, every one except **k, p, v, y** already
   means something:
   - a direction: n s e w f b l r u d;
   - a verb shortcut: i x o c g h j t q z m;
   - the article "a".

   Two-letter designators also collide: ne nw se sw fr br bl fl an mv. **Uppercase doesn't help with lowercase:**
   dq0c folds case as it makes its tokens, so `A` and `a` are the same token by the time the meaning is made. So:
   - The tokenizer must **keep the case of a one- or two-letter word** for designator lookup, while still folding it for
     dictionary words.
   - A designator must be recognised **by its position in the command**: a word in the noun-phrase position of the
     verb's frame. That needs the roles from the grammar (§4.2, step 7). Today's word-class scan reads `d` as a
     direction before it can read it as a designator. The interim rule while the scan remains: a one- or two-letter
     word after a verb that doesn't take a direction, or after an article, count, "the" or a preposition, is tried as a
     designator first.
   - **Verbs that take both a noun phrase and a direction** (throw, look, climb, jump, swim, move) need an explicit
     rule. `throw n` should mean *throw stack n* only if *n* is a current designator, otherwise north, and the reply
     should say which ("You throw the dagger (n).").
   - **"a" as an article versus designator a:** `drop a` means the designator; `drop a torch` means the article.
     Either the frame tells them apart, or *a* is never assigned as a designator (NetHack-style letter skipping).
   - **Ally names:** an ally named "Bo" or "Al" collides with a two-letter uppercase designator. names.c's clash check
     should refuse names of two letters or fewer.
6. **Stable lowercase letters.** NetHack's rule is that an inventory letter stays with its stack while you carry it.
   Store the letter with the item (a byte on `struct item`, or a map from item id to letter in the UI bank, saved
   with the game). A stack that splits keeps its letter on the part still carried; the part dropped gets an uppercase
   letter. Merging two stacks keeps the older letter.
7. **Extend uppercase letters to features later (optional).** Once doors, stairs, wells and the like are scope entries
   (step 4), they can be lettered too: "(C) a door (closed)", "(D) the stairs up". The pipeline needs no change. That
   would solve "the door behind me on my right" by giving the player a shorter way to say it. Whether to letter
   features is a presentation choice.

(§4.9 extends this scheme to containers and their compartments.)

So the designators aren't a fourth way of naming alongside noun phrases, names and "it". They are the visible face
of the scope table. **Noun phrases, designators, given names (`#id`), "it" and the target all resolve to the same
(handle, count), and nothing after the resolver ever sees a word.**

### 4.9 Containers and surfaces: everything that holds things

**Today.** *(2026-11-15, step 6b built: the compartment table below is dq0w/kinds.c's `comps`, a thing's place its
holder and its compartment; DECISIONS.md's "Compartments: containers and surfaces".  What follows is the survey as
it was.)*

- A kind has at most one receptacle: `KF_ON` makes it a surface, otherwise it's a container. A chest can't have
  things in it *and* on its lid. A desk can't have drawers.
- A thing inside another records only its parent item (`IW_ITEM`, x = the parent), not whether it's in or on it, or
  which part.
- A belt's slots, a sconce's torch, the floor (`IW_SQUARE`), the hands (`IW_ACTOR` + `IH_*`) and worn things each
  have their own representation.
- Reaching into carried containers is about 35 hand-written long-task steps (`LT_PACKOFF`, `LT_REACHIN`,
  `LT_OPENUP`, `LT_GETOUT`, `LT_PACKON` …). Each new kind of carrier means new steps and new wording.
  *(2026-11-11: those kinds are gone, ENGINE-CRITIQUE.md's step 6: a step is a record with an edge kind, `SE_*`, and
  its words a verb and a word, `SV_*` and `SM_*`; the tree it walks is still the one-receptacle model above.)*

**The model: one tree of holders and compartments.**

Every place a thing can be is a **compartment** of a **holder**, and every thing's location is exactly one
`(holder, compartment)` pair:

| Holder | Its compartments |
|---|---|
| a floor section (a square, or a room zone: "the corner", "by the door") | `on` |
| the character, and each denizen | left hand, right hand (`held`); back, body, head … (`worn`); and through the belt, its slots |
| a belt, a bandolier | `hung` slots (counted, as today's `slots`) |
| a sconce | `in`, one torch-family slot |
| a bowl, bucket, basket, sack | `in` (with what it keeps: liquid, grains, solids, as today's `keeps`) |
| a table | `on` (the top), and maybe `under` |
| a chest | `in` (lid, lock), and `on` (the lid's top) |
| a backpack | `in` main (flap), `in` side pocket (tie) |
| a desk | `on` top; `in` top drawer, `in` bottom drawer (each closes, may lock); `in` hidden compartment (hidden) |
| a trophy case | `in` interior (glass door: closes, **transparent**); within it, `on` top, middle and bottom shelves |

Compartments can be **inside compartments**: the case's shelves are reached only through its interior. That's the
"containers with internal surfaces or containers" case.

**The data.** One compartment table in the catalogue's bank, indexed per kind (first, count):

```c
struct comp {                 /* ~8 bytes; a kind's list, in order */
    unsigned char noun, adj;  /* "drawer"/"shelf"/"top"/"pocket"/"lid"; "top", "bottom", "left", "secret" … */
    unsigned char prep;       /* CP_IN, CP_ON, CP_UNDER, CP_HUNG, CP_HELD, CP_WORN */
    unsigned char parent;     /* the compartment it's reached through (0xFF: the holder itself) */
    unsigned char closes;     /* none, lid, door, drawer, flap, tie, stopper; | lockable */
    unsigned char cap_bulk, cap_wt;   /* (the kind's cap_bulk/cap_weight move here, per compartment) */
    unsigned char flags;      /* HIDDEN, TRANSPARENT, FAMILY(n) (a scabbard's swords, a sconce's torch), SLOTS(n) */
};
```

- **The location** stays the item record's `where`/`x` (the holder: a square, an actor, an item) plus a
  **compartment index** in a field IW_ITEM doesn't use today (`y`, or `sn`).
- **Each instance's compartment state** (which drawers are open, locked, burst, hidden ones found) needs a few bits
  per closable compartment. Two bytes per item covers four closable compartments, which is enough for furniture. The
  current `shut` byte becomes compartment 0's.
- Most kinds have one compartment. A sack is one row, so the catalogue grows little and existing kinds convert
  mechanically (`KF_ON` → one `CP_ON` row, a container → one `CP_IN` row, `slots` → `SLOTS(n)`).

**What the tree gives the rest of the design.**

- **Visibility and reach are worked out from the path, not coded per case.** A thing can be seen if every
  compartment on its path is open or transparent (and lit). It can be reached if every one is open, within reach,
  and not on a worn back. The trophy on the case's top shelf is visible through the glass but not reachable until
  the door is opened.
- **The reach becomes a path planner** (§4.4's preconditions phase). To get from the hand to a compartment, walk the
  tree:
  - a holder worn on the back → take it off;
  - a closed compartment → open it;
  - each level → reach in;
  - afterwards, put it back on.

  Each edge is one step with its time and its first words, built from the compartment's `closes` and noun ("You open
  the top drawer.", "You reach into the side pocket."). Today's ~35 `LT_*` reach steps become a handful of edge
  kinds that work for any carrier, including a denizen's (a robber reaching into the character's pack), because the
  planner doesn't care whose tree it is.
- **Every verb about places reads compartments**: put/take/look in/look on/search/open/close/lock/unlock/empty, plus
  "take everything from the drawer" and "empty the sack onto the table". Search goes through every compartment
  (hidden ones by the referee's test, then known from then on). Look and examine list compartments only when
  examined or when they hold something ("On the table: …"), so the room's description stays readable.
- **Capacity, form and family rules move to the compartment**, so a sconce takes one torch, a scabbard a sword, a
  bucket water, a basket no sand: data, not special cases.

**Designators with containers.** The lexical scheme is **[count] letter(s) [compartment number]**. It's unambiguous
because counts go before the letters and compartment numbers after:

```
c        stack c                 3c       three of stack c
b        the backpack (as a thing: "wear b", "drop b")
b2       the backpack's compartment 2 (its side pocket): "put 3c in b2", "look in b2"
D3       the desk's compartment 3 (the bottom drawer): "open D3", "take the key from D3"
```

- **Stacks get letters; compartments get numbers.** Compartment numbers are shown only where a holder has more than
  one compartment, and only when it's examined or listed, so most of the time the player sees only letters.

  ```
  (b) a backpack, worn
        b1 main (open): (c) 3 torches, (d) a flask of oil, (e) a tinderbox
        b2 side pocket (tied)
  (f) a belt, worn: (g) a pouch (in it: (h) 20 pennies), (i) a sheath (in it: (j) a knife)
  ```

  ```
  > examine D
  (D) a writing desk of oak.  D1 its top: (E) an inkwell, (F) a sheaf of paper.
  D2 the top drawer (closed).  D3 the bottom drawer (closed, locked).
  ```
- **A lowercase letter stays with its stack at any depth** while it's carried (NetHack's rule): moving a torch from
  the pack to the belt doesn't change its letter.
- **Things inside an environment container get uppercase letters once seen**: a chest opened, a table's top, through
  the case's glass. Things in closed or unseen compartments have no letters, because the character doesn't know of them.
- **A single-compartment holder's letter alone means its compartment** when a place is wanted: `put c in G` where
  *G* is a sack. So the numbers are needed only for multi-compartment furniture and carriers.
- **Floor sections aren't lettered.** They're named by the locatives the game already understands, which become the
  floor holder's qualifiers: "on the floor", "at my feet", "ahead", "in the corner", "by the door". Optionally, a
  look could letter a zone that has things lying in it.

**The stretch goal: `put the trophy in the trophy case on the top shelf`.** This needs two things, and both fall out
of the model:

1. **A noun phrase with a chain of locatives.** The noun-phrase rule (§4.2) allows any number of
   `(in|on|under|inside|from|of) NP` qualifiers, and the possessive forms "the case's top shelf" and "the top shelf of
   the case". The parser doesn't decide what they attach to. It passes the chain on.
2. **The resolver reads the chain against the tree.** For `in the trophy case on the top shelf` there are two readings:

   | Reading | Means | Holds when |
   |---|---|---|
   | **Path down** | the destination is a compartment *of* the case: its top shelf | the case's kind has a compartment with noun "shelf", adj "top" |
   | **Location** | the destination is the case, and "on the top shelf" says *which* case (one standing on a top shelf) | some trophy case lies on some top-shelf compartment |

   The rule is to try path down first, since a phrase naming a part of the thing before it nearly always means that
   part, then location, and to ask only if both hold and give different destinations. The final preposition comes
   from the compartment ("on" the shelf), so `put the trophy in the case on the top shelf` and `put the trophy on the
   top shelf of the case` give the same command.

   Then §4.4's phases run as for any other command: the shelf is visible through the glass; the planner chains *open
   the case's door → put the trophy on its top shelf*; capacity is checked on the shelf's row; the report says "You
   open the trophy case and set the trophy on its top shelf."

The same chain-reading gives `take the apple from the bowl on the table`, `look in the top drawer of the desk`,
`the sack in my left hand` (HOUSE.md's "by where", not built), and `drop the spent torch from the side pocket`. These
are the possessive and "which one by where" forms the decisions deferred (DECISIONS.md: possessives "later, in a stage
of their own").

**What it costs.**

- The compartment table: a few hundred bytes for today's kinds plus furniture.
- Two bytes per item record for per-compartment state. 744 records × 2 is about 1.5K, in a store that's already
  bank-paged. It's the biggest single cost, and could be confined to kinds with more than one closable compartment
  (furniture as a record type of its own) if the banks are tight.
- The chain resolver is a small loop over the scope table's parent links.
- Against that, the reach's per-carrier step code and its wording tables shrink to the planner's edge kinds.

---

## 5. Getting there from here

This is incremental. Each step leaves the game working, and the early steps fix what players see.

| # | Step | Fixes | Size |
|---|---|---|---|
The designators are being built now, so the order is arranged so that they lay the foundation instead of adding
another layer. Steps 1–3 are what the designator work needs anyway. Merge them into it, or land them right after it.

| # | Step | Fixes | Size |
|---|---|---|---|
| 0 | **The action × class sweep test** (§4.6) against today's game, recorded as a baseline | measures the problem | small |
| 1 | **Handles travel.** The item's permanent `id` goes in `ireq`, the actor slot or denizen number in `sway`; the world and the denizens act on that one thing when it's given, and choose only when it isn't. *Designators need this to mean anything* (§4.8 point 1) | the double-choice bugs (torch playtest, "which rat? 2"), and `drop c` dropping another torch | small |
| 2 | **The scope table, first cut**: one list of what's in sight (denizens, then things) and what's carried, by stack, each with `cls`, `ref` (permanent id or slot), noun, state, where. The designators are assigned on it, and lowercase letters are stored by item id | one list for the letters instead of a letter column per array | medium; **this is the designator work, built on the right base** |
| 3 | **(handle, count) out of the resolver**: designators with counts, noun phrases, `#id` names, "it" and the target all produce the same result; the "which?" questions list designators and take one as the answer | "3A", "take 2" as a count only, one question path | small–medium |
| 4 | **Never drop an object silently.** In `s_command`, an action whose handler didn't use a named object gets a refusal instead of its no-object version (climb, sit, smell, light, look at me or the stairs) | about half of §2 | small |
| 5 | **The refusal table** (§4.5), and the scattered messages moved into it | consistent wording | medium |
| 6 | **Features in the scope table**: stairs, walls, wells, sconces, portals become entries with affordances and noun ids; the item-or-feature rule made explicit; optionally lettered (§4.8 point 7). resolve.c, choose.c's naming and things.c's `pick` for words are retired | `take the door` → "fixed", `look at the stairs`, the remaining choosers | medium |
| 6b | **Compartments** (§4.9): the compartment table in the catalogue, `(holder, compartment)` locations, per-compartment state; existing kinds converted one row each; then furniture (desks, cabinets, cases) added as data. *Done 2026-11-15 but for the furniture (DECISIONS.md's "Compartments: containers and surfaces"): dq0w/kinds.c's `comps`, a row a compartment, every kind that holds things one row (its record's, the build holding them the same), a chest two (its inside, its lid's top); a thing's compartment in its record's y (`IC_MARK`), each compartment's closed bit in its holder's shut; put, take, look in and on, search, examine, the reach's levels and the senses by compartment. The second stage done 2026-11-16, the user's answers (DECISIONS.md's "Compartments: the second stage"): every reader of a thing's place by its compartment (an ally's errand, the chooser, "the empty chest" and the letters', where to stow, the nested lists), a lid's top by its holder's size, things on a lid sliding off as it's opened, a compartment named by its own name through the one name match ("open the drawer", "put the coin in the side pocket"), the backpack's side pocket, locks on compartments past the first, hidden compartments (a false bottom, a secret niche) found by a search, furniture (a desk, a cabinet, a case, shelves) as data and placed by the maker's placers.  Left: `b2`/`D3` designators, the maker stocking furniture's compartments, the denizens' reach by compartment* | in/on per kind, drawers and shelves, hidden compartments, `b2`/`D3` designators | medium |
| 6c | **The reach as a path planner** (§4.9): today's `LT_*` reach steps replaced by edge kinds walked from the tree. *Under way 2026-11-11 (ENGINE-CRITIQUE.md's step 6, DECISIONS.md's "The step planner and the activity stack"): the `LT_*` reach steps gone, a step a record with its edge (`SE_*`: off, in, on, drop, out, light, stow, up, hand, back in, place, sling) and its words a verb and a word (`SV_*`, `SM_*`); the walk still knows a container by its kind's flags, one level a thing, waiting on 6b's compartments for `(holder, compartment)` and each compartment's `closes` to choose the edge. 6b built 2026-11-15: the walk reads each level's compartment (`rs_walk`'s `rs_wc`: a chest's lid's top no level, its lid shut or not); the edge still chosen by the kind's flags* | any carrier or furniture reached the same way, a denizen's too | medium–large |
| 7 | **The dispatch table** (§4.4). Cases are moved into handlers one action at a time; the coverage report goes into the build | the remaining combinations | large, but parallelisable |
| 8 | **Roles from the grammar** (§4.2). Designators recognised by position, case kept for one- and two-letter words; `scan()` and the `strstr(cl…)` readers retired; one noun-phrase rule with chains of locatives and possessives | the letter collisions properly (§4.8 point 5), `<object>`/`<thing>`, "I don't understand" versus "not yet" | medium (assembly) |
| 9 | **The chain resolver** (§4.9's stretch goal): locative chains and possessives read against the tree, path down then location | `put the trophy in the trophy case on the top shelf`, `the sack in my left hand` | small, once 6b and 8 are in |

### 5.1 The one resolver, against the code as it stands (2026-11-07)

A survey of the code after designators steps 4–7 found **six choosers** deciding what a name means, not four:

| Chooser | Chooses among | Its rule for several | Asks "which?" |
|---|---|---|---|
| resolve.c (overlay 11) | things lying about and denizens in sight (`sight_copy`) | nearest, or largest, smallest, nth by selector | for move, approach, target and throw-at only |
| choose.c (overlay 7) | doors, openings, passages (`door[]`, `open[]`) | the one directly ahead | a numbered list; answered only by a number or a way |
| letters.c (overlay 22) | carried things (lower case), things and denizens seen (capitals) | exact: a letter is one id | "Drop what? [a, d, f]" |
| caps.c (overlay 23) | gives the capitals; words the world's "which one?" | none: the world chose | "Which one? (E) a torch (lit), (F) a torch" |
| sway.c's `ix_pick` | carried things, to say the first words at once | a copy of the world's ranking, carried things only | no |
| the world's `pick` (dq0w/things.c) | the item store: carried, lying about, remembered | held 0, carried 1 + depth, lying 8 + distance; nth | returns IR_WHICH for caps.c to word |

**Where they disagree**, each with a case a player can hit:

1. **The resolver's choice was erased.** It found the thing, then sent the world the bare name with the selector
   reset, and the world chose again. "take the closest torch", with a lit one at your feet and an unlit one a step
   off, asked "Which one?". *Fixed 2026-11-07* (step A below).
2. **"the second" has three orders:** the resolver's left to right by heading (and only if a denizen is among the
   candidates), the world's by most to hand, and `ix_pick`'s by carried rank only. *Settled 2026-11-07:* a carried thing's nth is the UI's, made final (step C); things lying about, the world's; denizens, the resolver's: one order for each kind.
3. **"largest" and "smallest"** exist only in the resolver. *Fixed 2026-11-07* (step A: the resolver's choice is the world's).
4. **The state selectors differ** between `ix_pick` and the world. "open": the world also needs the kind to close.
   "full": the world rejects anything that's not a container. "spent": the world also takes an empty container. So
   the first words can name a different thing from the one the world takes. *Fixed 2026-11-07* (step C).
5. **The guess at the world's "which?"** ("You look them over.") counts only things in reach, matched by `strstr`.
   The world counts remembered and out-of-reach ones too, matched by its own rule, and needs two that differ. *Fixed 2026-11-07:* said only when two seen differ, as the world's which asks (test dq0leftovers).
6. **Six name-matching rules:** whole word with plural; prefix and suffix; `strstr`; the first three letters for a
   denizen. "pack" finds the backpack in the world but nothing in the resolver. *Fixed 2026-11-07* (step F).
7. **A capital for a thing lying about** words its first words from a carried thing of that name. *Fixed 2026-11-07:* a handle's word is never worded as another of its name.
8. **"it" keeps a word, not the thing:** "drop it" after a letter asks the world by name. *Fixed 2026-11-07* (step B).
9. **The instrument's selector is overwritten** by the object's ("… with the other sword"). *Fixed 2026-11-07:* the weapon taken in hand by its own selector.
10. **Four "which?" answer parsers**, each with its own state: the resolver's (selector, number, name, way),
    choose.c's (number or way, and only for moves), letters.c's `which_in` (capital, digit, state word), and
    `what_ask`'s (one letter). *Fixed 2026-11-07* (steps D and E: one reader, which_in; "Drop what? [a, d, f]" keeps its own, a question of a different kind).
11. **Attacking one of several denizens** isn't asked about; blows.c takes the first adjacent one in table order. *Fixed 2026-11-07:* the resolver chooses the nearest, its square the fight's to go by.
12. **"take all"** lists the things nearest first, then takes each by name, so the world may take a different one.
    *Fixed 2026-11-07* (step B).

**Why the UI must stay the chooser.** The first words are said at once, before the world's answer arrives some 2M
cycles later (D17: "Word as they begin."). So the UI can't ask the world to choose first. It doesn't need to: it
already has what it needs. What's carried is in the UI bank's `ix`, each with its letter and so its id through
`low[]`. What's seen is in `sight_copy`'s items, each with its store record and so its id. The plan is for the UI's
choice to be final, sent as a handle: `#id` for a thing, the actor slot for a denizen, the square for a feature. The
world would choose by name only for what the UI can't see (things remembered in the dark) and for the minds.

**The order of work:**

| # | Step | Retires | Fixes |
|---|---|---|---|
| A | **The resolver's choice of several travels** as the thing's id (letters.c's `des_seen` adds it to the line's handles, as a letter's is). *Done 2026-11-07, test dq0closest.* A single candidate still leaves the choice to the world, whose rule puts what's in hand first | — | 1, part of 3 |
| B | **"it" and "take all" carry ids:** `last` keeps the handle, and "take all" queues ids. *Done 2026-11-07, test dq0it:* the last thing named keeps its id where one's known (`it_id`, the main RAM's: a letter's, the resolver's choice, or the carried thing sway.c's `bind` chose), and "it" (or a verb's unsaid thing, the last named) makes it the line's handle again, if it's still carried (the UI bank's `low`) or in sight, else by its name as before (a thing gone, or joined to a pile, has no id to name); "take all" reads each listed thing's id from its store record as it lists them, the first the line's handle, each next one's set as `taken_next` hands it on (the line's handles let go then). The handles' adder, letters.c's `des_add`, is in overlay 24 | the by-name re-requests | 8, 12 |
| C | **One chooser for things**, in the UI, over carried (`ix`) and seen (`sight_copy`) together, ranked by the world's order (held, then carried by depth, then lying by distance), honouring every selector, with the state selectors decided by the kind's flags (`ix_make` to send the flags the world uses). *Its first half done 2026-11-07:* the carried thing the first words choose (sway.c's `ix_pick`) is made final, its id sent as a letter's is (sway.c's `bind`), for put down, hold, stow, put, wear, take off, light and put out. *With a state said too, 2026-11-07* ("the spent torch", "the full pot", "the open sack"): `s_ix_make` sends the flags the world's chooser tests (`pick_`'s and `unlike`'s), so `ix_pick`'s state tests are the world's (test dq0statesel); but not to light or put out, as the world's `light` chooses by the name and not by `pick_` ("light a new torch" lets the old one fall by its name). Not with another thing named ("the torch in the sconce", "the sack in the sack") but a put's of another name | resolve.c's item gathering, sway.c's `ix_pick` and the "look them over" guess | 2, 3, 4, 5, 7, 9 |
| D | **One "which?"**: one state, one wording (capitals, as caps.c's), one answer parser (a capital, a number, a way, a state word, a name), used by the thing chooser, choose.c and the world's IR_WHICH. *Done for things and denizens 2026-11-07:* the resolver's question is asked by the designators' capitals (which.c, overlay 24: "Which one? (A) the giant rat ahead to your left, on the right, (C) …"), answered by letters.c's which_in (a capital, its number, a way, "closest", a name or state word, "the" before it), the command made again on the chosen capital; the resolver's own question and kept command gone (overlay 11: 7 bytes free to 1,097). Doors and openings joined in step E | resolve.c's `ask_which` and `answered`; choose.c's question and answer | 10, 11 |
| E | **Features in the same list:** choose.c's doors and openings as entries, perhaps lettered after the denizens, so stairs and doors are chosen and asked about the same way *Done 2026-11-07, without lettering them* (that would change every room's words, and the designators gave capitals to things and denizens only): choose.c's question is asked through which.c ("Which door? (1) the door ahead to your left (closed), (2) the one to your right (open)"), each choice's square and way and a few words in the same which_w slots, answered by the same which_in (a number, a way, "closest", a word: "open", "double"), the line made again with the chosen square in main.c's pick_x and pick_y for o_pick; choose.c's own answer and kept command gone (overlay 7: 40 bytes free to 255). Its choosing stays its own, by the kind of thing, as nothing else chooses doors | choose.c's question and answer | §3.4's |
| F | **One name match** (the world's rule: word prefix, plurals, suffix of a one-word name; qualifiers checked), shared by all. *Done 2026-11-07, test dq0namematch:* the UI's `name_is` (nameis.s, the main RAM's, in assembly: 366 bytes, the C was 550) is the world's `named` (a word of the name or its start, the word made plural by s or es, the end of the name for a word longer than 3) and the qualifier check (the word before it, "black", "giant", a word of the list or its start, in the name too, by the same rule). Used by the resolver's gathering (things and denizens, and the denizens remembered), the first words (`ix_pick`'s kinds, `o_walk_ahead`'s things seen and "You look them over."), the blow's denizen (blows.c), the tending's patient (tend.c) and a walk's new things (task.c). The world's `named` took `named4`'s end-of-name rule and its copy went, so the chooser, the light, the market and the search ask one rule. Left with their own: `o_may_den` (three letters anywhere: looser, only a "maybe" that asks the denizens), letters.c's `which_in` (a word in a "which?" choice, not a name), main.c's `bow_named` ("bow" names a shortbow there; the rule's end-of-name wants 4 letters) | the other five | 6 |

**What step C taught (2026-11-07).** The UI can be the final chooser only for what it sees completely: what's
carried (its `ix`, every entry with an id). Things lying about can't be chosen finally by the UI: the sight record
holds six, and the world's chooser also sees things remembered in the dark and things on a surface. And a second
copy of what's carried, made in another overlay, cost the first words some 30,000 cycles, past the user's 720,000:
so the choice is made final where the first words already make it (sway.c), not in a chooser of its own. The
state selectors, bound, broke dq0torchwords not by the flags (which disagreed too, and agree now) but by "light a new torch": the world's
`light` lets the old torch in hand fall only if it's of the name asked, and "#id" names none. Overlay
24 is added (part 6's image moved into it by the bank packer as overlay 20 grew), for step D.

**§5.1 done, 2026-11-07.** Every step and every disagreement above is closed or settled, each held by a test (dq0closest, dq0statesel, dq0it, dq0namematch, dq0leftovers, and dq0cmd's and the door tests' questions). Overlay 24 holds the shared question (which.c) and the handles' adder; overlay 11 (resolve.c) went from 7 bytes free to some 1,300, overlay 7 (choose.c) from 40 to 255.

**Room.** Steps C and D should remove more code than they add (resolve.c's gathering, `choose_one`, `ask_which` and
`answered`; choose.c's question; sway.c's guess). But old and new must run side by side while each case moves over,
and the UI's banks are full, so step C will likely need a staging overlay bank, as overlay 23 was for the designators.

### 5.2 Dispatch, against the code as it stands (2026-11-07)

A survey of the dispatch after §5.1 (every verb, where it's handled, what it asks of the world, every refusal):

- **The chain is cut by size, not meaning.** A command goes `s_command` (part 2, 2,145 bytes) → `chores` → `more`
  (overlay 16) → `rarer` (part 24) → "You can't … in this game yet."; `missiles`, `look_way`, `search_go`, `hide`
  and others hang off the switch. Some cases are unreachable (chores' VB_OBSERVE_OBJECT); 42 verbs reach no handler.
- **The same steps are written over and over, each a little differently:**
  - a thing: ask the world, walk over if it's out of reach, ask which if several, word it, charge the time. `handle()`
    does all of it; some fifteen other handlers each do part (drop no "which", take its own walk, hide no reach);
  - out of reach, then the command again: four implementations (`reach_go`, `door_go`, take's own, `go_thing`'s);
  - a door or a feature: find it, walk up, face it, first words, act: one full path and seven partial copies, with two
    ways of turning (an intent, and way.c's own);
  - refusals: "You see no %s here." built in eight places; "out of reach" worded four ways; a dozen "<verb> what?".
- **Commands that ignore what they name, beyond the first sweep's verbs** (test dq0sweep2: eat, drink, fill, look in,
  tie, untie, wear, push, pull, unlock, lock, raise, lower, disarm, force, bend, bar at every kind of thing): 55 of 136
  lines. "eat the pot" said "You're not hungry."; "raise the pot" "There's no door that way."; "disarm the pot"
  "There's no trap you know of within reach."; "wear the stairs" "Which thing? Name it …".

**The order of work:**

| # | Step | Fixes |
|---|---|---|
| G | **The second sweep, and its gaps closed.** *Done 2026-11-07:* refuse.c's rows for eat, drink, fill, tie, untie, wear and look in at a feature, and for raise, lower, bend, bar and disarm of a thing by its word (but a door's or a window's part: "lift the bar"); feed.c refuses a thing named that isn't food or water, and names it when not hungry or thirsty. dq0sweep2: 0 of 136 | the commands that ignore what they name |
| H | **One path for things:** every verb that acts on a thing through `handle()`'s steps (the world asked, walked to, which asked, worded, timed). *Done 2026-11-07, test dq0onepath:* the asking, the walk over and the "which?" are one routine, main.c's `things` with `IO_GO` (out of reach, `reach_go` and the command again; several that differ, `which_ask`; either, `IR_GONE`, nothing more for the verb to do). Used by handle (hold, stow, put, take from, wear, take off, light, put out), take, drop, hide (the thing, and what it hides behind), search for or on a thing, look in, fill, tie, open/close/lock a thing, look at (way.c's `looked`), appraise (`judged_as`), read, climb down a well, and a blow's weapon taken in hand (`hold_first`). Each verb keeps its own words and its own special cases (take's portal part, drop's counts, hide's cover). A thing lying about that's hidden is taken up first, then hidden. The time stays each verb's own: `after()`'s light check matters only to the handling verbs, and sharing it would swap part 2 in for every look. Left out: the throw (its thing is carried, its mark is a target, not reached), a denizen's gear (the sway's), the theme's puzzle (its own PZ_FAR, already `reach_go`'s) | hide, drop, take, search, look in, fill, read each doing part |
| I | **One reach:** take's own walk, `go_thing`'s and `place_go`'s made `reach_go`'s. *Done 2026-11-07:* `reach_go` is the one walk over, by `walk_a` (I_TAKE's a): 1, the command kept and carried out again (every verb, and `door_go`'s I_GOTO); 0, take's (the world takes the thing up as it gets there, E_TAKEN); 2, a "go to" (way.c's `go_thing`: walked on as it gets nearer, nothing after). Take's own message and way.c's own I_TAKE are gone. Kept apart on purpose: a denizen's approach (I_CLOSE, `go_thing`'s and a blow's: up to it as it moves, the fight's), and `place_go` (a place in the room, not a thing: its own words, "You set off across the room.", and nothing after) | the walk-overs |
| J | **One path for features:** find, walk up, face (one turn: the intent), first words, act; way.c, `s_lift`, the errand's door through it. *Done 2026-11-07, test dq0features:* main.c's `feature` (part 13, `s_door_go`'s place) finds (choose.c's `pick`), walks up if it's out of reach (one test, by what the last step needs: the world's door work reaching, the default; beside it, `FG_NEAR`, for a close look the UI makes itself; never, `FG_FAR`), takes a blow's thing in hand, faces (the intent, `intend2 (I_TURN)`; not for a move, `FG_GO`, which turns as it goes) and says the first words; each command its own last step. Used by the portal path (door verbs, windows climbed through, check for traps, search, attack, climb), the move to or through a door or a passage, leave, way.c's close look (`inspect`) and look, listen and sniff one way (`s_look_way`), and `s_lift` (its door found by the part's square, then faced through it). way.c's own turn (`turn_to`, a bare hub_send) and `inspect`'s own reach test are gone. The errand's door (errand.c) and the blow's "is there a door" (choose.c's `s_wield`) only find: the character doesn't act on it; a look at stairs (way.c's `opened`) only finds, as before (the words say where they lie) | the copies of the door path |
| K | **Refusals as data:** one table of answers with the thing's name put in ("You see no %s here.", out of reach, "<verb> what?"), refuse.c's rows its first part. *Done 2026-11-07:* refuse.c's `said[]` (overlay 24 now, overlay 1's room) is the one table, U_* then A_* (dq0.h): the line's refusals, "You see no %s here.", "You have no %s." (each with a nameless form, said when there's no name to put in), "There's nothing here to take.", "You haven't seen a(n) %s here." / "that way.", "%s is out of reach: go up to it first.", "You can't get near enough to %s.", and "%s what?" from the line; said by `say_no (a, w)` (or `unknown (U_*)` from the line), or added to lb (A_LB: the sway's words, a blow's note). Thirty-five sites in fourteen files call it in place; five wordings of out of reach became two (reach, and getting near); "You see none of those here." and "You carry none of those." name the thing now; "Take what?", "Put down what?", "Read what?", "Copy what?", "Name what?" are the line's verb with "what?", as "Drop what? [...]" was already. Left as they are: the questions with a hint or another shape ("Throw it at what? ...", "Give it to whom?", "Fetch what?", "Name it what?", "Use what, and on what? ...", "Combine what with what? ..."), the "Which one?" headings (a question's start, its choices after), talking range ("... is too far off to hear you."), and §4.5's per-action rows (the U_* rows of refuse.c's rules[] are what there is of them) | the same refusal worded many ways |
| L | **The dispatch table:** verb → handler by meaning, with the kinds of thing each takes; the chain by size (`chores`, `more`, `rarer`) retired. *Done 2026-11-08* (below): stage 1, the table and one dispatcher, the cases its handlers unchanged; stage 2, refuse.c's rows in it; stage 3, the handlers in files by meaning (hthings.c, hplaces.c, hmove.c, hdens.c, hself.c: cmds.h), each group reached by its entry and the handler's number, `run16` and `run24` gone, lock's request its row's; *the rest done 2026-11-15*: the handling verbs' requests in their rows, the coverage report in the build | the split by size; the unreachable cases |

**Step L, the dispatch table, as planned (2026-11-07).** After H to K, each verb is one handler and the per-class
paths are shared. L then replaces the chain cut by size with one table read by meaning:

- **A row a verb** (`VB_*`): its handler's number, the kinds of thing it takes (a carried thing, a thing lying about,
  a denizen, the feature kinds as refuse.c's masks are, a direction, nothing), its request (`IO_*`, `PA_*`, `I_*`:
  today's `cmd.aux`), and its refusals for a word and for a kind (refuse.c's `rules[]`, folded in). About four bytes
  a row, some 130 rows: in an overlay, not the main RAM.
- **One dispatcher** in `s_command`'s place: the row found; a thing named that the row doesn't take refused from the
  row (step K's table); else its handler called through a table of the handlers' stubs (main-RAM addresses, so a
  pointer to each is good whichever part or overlay the handler's in).
- **Handlers by meaning**, each a function of its own: the things' verbs (H's path), the features' (J's), moving,
  the denizens' (the sway verbs, the fight), the body's, the meta commands. Where each lives is the build's: its
  pragma and the packer, not "the room left in part 2". `chores`, `more` and `rarer` go.
- **Verbs with no handler** (42 today) get their row's refusal, "not yet", by the same path, and adding one is a row
  and a function.
- **Order:** first the table and dispatcher calling today's cases unchanged (the switch's cases made functions); then
  refuse.c's rows into the table; then the cases moved into files by meaning, one group at a time, the suite after
  each.

**Step L, stage 1, as built (2026-11-08).** The table and one dispatcher, today's cases its handlers, nothing a player
sees changed (the whole suite):

**Step L, stage 2, as built (2026-11-08).** refuse.c's rules[] folded into the rows: each row's third byte its refusals (a word's, its high four bits with W_CLIMB and W_PART; a kind's, its low four), its takes' high four bits which of masks[] the verb does; o_refused beside the table in overlay 11, a row found by its verb at once, not a list searched (31 verbs' refusals; refuse.c keeps only the words, its said[] and o_unknown: overlay 24 196 bytes the freer). Behaviour unchanged, the suite's proof.

- **The table** is actions.c's `rows[]` (overlay 11, beside the resolver: 738 bytes left there), a row a verb in
  actions.h's order, 142 rows of four bytes: the handler (`H_*`, dq0.h), the kinds of thing it takes (`TK_*`), and two
  bytes kept for the later stages, its refusals (stage 2) and its request (stage 3). Each row is written
  `ROW (VB_*, H_*, TK_*)`; games/dq0/make.js holds the rows to actions.h's order (the build stops if they differ, as
  dq0c's make.js holds actions.h to the catalogue). `verb_row (a)` (an overlay stub) reads one: the handler, and the
  takes times 256.
- **The dispatcher** is the end of `s_command` (part 2): after the resolver, refuse.c and the puzzle, the row read and
  a switch on its handler number. Part 2's handlers are called straight; overlay 16's through one stub, `run16 (h)`
  (words.c's, `more` till now); part 24's through one, `run24 (h)` (main.c's, `rarer` till now). Each of those two
  switches on the handler number, not the verb. No table of addresses: a stub a handler would cost the main RAM 6
  or 11 bytes each (some 35 handlers), and a switch on ~25 numbers costs the answer nothing that shows.
- **The handlers:** part 2's are s_command's cases and chores', made functions where they're more than a call
  (`do_go`, `do_climb`, `do_face`, `do_door`, `do_examine`, `do_take`, `do_drop`, `do_missile`, `do_inventory`,
  `do_fight`, `do_feel`; and `do_portal`, the door's work that five of them end in, the old `portal:` label, and
  `do_put`, the old `put:`), the one-call ones left as a line of the switch (look one way, wait, stop, help ...).
  Overlay 16's: the time, fill, look in, tie (falling into the shut as before), eat and drink, tend, and two with no
  verb of their own, `H_SHUT` (a thing that closes, first for the door verbs whose row has `TK_CLOSER`: open, close,
  unlock, lock, force) and `H_HIDDEN` (a hide begun, the hide's own). Part 24's: dig, jump and climb (one body, as
  before), the sway verbs, give and trade.
- **`chores` is gone**, its cases part 2's handlers. **`more` and `rarer` dispatch nothing now**: called only for
  their own handlers' numbers, by the table, so which function sees a verb is its row's, not the order they were
  tried in. 50 verbs' rows say `H_NOT_YET` ("You can't ... in this game yet."): the survey's 42, and 8 seen to before
  the table (the pace's step, set pace and pace; the target's three, resolve.c's; use, read and combine, the puzzle's),
  as noted in their rows.
- **Unreachable cases dropped:** chores' `VB_OBSERVE_OBJECT` (s_command's examine case took it first); more's open,
  close, unlock, force and lock as verbs (s_command's door case took them; their work is `H_SHUT`, which the door
  handler calls as before) and its `VB_HIDE` (s_command's hide took it; its work is `H_HIDDEN`); rarer's `VB_CLIMB` as
  a verb (s_command's climb took it; its work is part 24's `H_CLIMB`).
- **Room:** the main RAM 2,133 bytes free to 2,122 (the one new stub, `verb_row`; `more` and `rarer` became `run16`
  and `run24`). Part 2 905 to 1,059 (the switch on ~25 handler numbers, not ~70 verbs, and chores' gone); part 24 3,238
  to 3,342; overlay 11 1,367 to 738 (the table); overlay 16 146 to 184.
- **What stage 2 needs:** refuse.c's `rules[]` folded into the rows' third byte (a word's refusal, a kind's: `U_*`, four
  bits each) and the takes byte (the kinds a verb does, its `masks[]` index, three bits, beside `TK_CLOSER`), and
  `refused ()` asked from the row (`verb_row` giving all four bytes, or refused's test moved to overlay 11, the row at
  hand). `W_CLIMB`'s and `W_PART`'s exceptions are bits of the takes too.
- **What stage 3 needs:** the handlers moved into files by meaning (the things', the features', moving, the denizens',
  the body's, the meta commands). A handler moved is its pragma and its number's place: `run16` and `run24` are each a
  bank's one entry, so a handler moved to another part or overlay moves its `case` to that bank's entry (a new bank's
  entry, one stub). The fourth byte, the request, where an action's `cmd.aux` is the same for every phrase (the door
  verbs' `PA_*`, the handling verbs' `IO_*`), so the handlers needn't read the verb.

**Step L, stage 3, as built (2026-11-08).** The handlers in files by meaning, each group's place its pragma's, nothing a
player sees changed (the whole suite: all 426 tests pass):

- **The groups** (cmds.h lists them, and what they share): `hthings.c`, the things' verbs (take, drop, handle: hold,
  stow, put, hang, take from, wear, take off, light, put out; hide; eat and drink) and their timed works (fill, look in,
  tie and untie, a thing that closes worked, `H_SHUT`, the door verbs' first; a hide begun, `H_HIDDEN`); `hplaces.c`,
  the features' and the doors' (a door's work, `portal`, which five handlers end in; climb; examine; search; a look,
  a listen, a sniff one way, a track, leave); `hmove.c`, moving (go, enter, approach; face; jump); `hdens.c`, the
  denizens' (a blow and the stance, the missiles, disengage and flee, the sway verbs, give and trade); `hself.c`, the
  body's and the meta commands (the limbs, a feel about, a look, wait, stop, tend, dig; inventory, sheet and map, time,
  pause and save, help, quit). dq0.h's `H_*` are in that order, a group's numbers together (`H_WORK`, `H_PLACES`,
  `H_MOVING`, `H_DENS`, `H_SELF` each group's first).
- **Where each is:** all but the timed works in part 2, the commands' own (s_command's): a handler there costs no copy
  of a part between the line and its answer, and part 2 had the room (1,059 bytes). The timed works in overlay 1 (words.c's
  in overlay 16 till now: overlay 16 was 131 bytes from full, overlay 1 had 1,288), through one stub, `things_work`, as
  `run16` was: the hide (part 18) calls it too, and a part's call into part 2 would cost two copies of a part where an
  overlay's costs ~100 cycles. A group moved is its pragmas alone: a part's entry is `s_NAME`, called as `NAME`, the
  build's routing (straight from inside its part, through its stub from outside: one stub, 6 bytes of the main RAM).
- **The dispatcher** (s_command's end): the row read, then its group's entry by the handler's number, five compares
  (`self_do`, `dens_do`, `moving_do`, `places_do`, `things_work`, `things_do`; `H_NOT_YET` "not yet"). Each entry
  switches on the number, as `run16` and `run24` did. No table of stubs: all the part-2 entries are called straight, and
  one stub is all the overlay's needs.
- **Gone:** `run16` (overlay 16's entry, its stub) and `run24` (part 24's, `s_run24`, its stub): part 24's handlers
  (dig, jump and climb, the sway, give) are part 2's now, so a dig, a jump, a climb, a sway, a gift or a trade no
  longer copies part 24 in and part 2 back (`deal`, the sway's asking, is still part 24's, through its stub, as the
  examine of a denizen always called it). `do_climb`'s 0x80 (through a window, part 24 back to part 2's portal) is
  gone with it: the climb works the portal itself.
- **The request, the rows' fourth byte:** filled for the door verbs, whose `PA_*` is the same whatever's said (open,
  close, unlock, lock, break, raise, lower, bend, force, disarm; not bar, whose "unbar" is `PA_LIFT`), and `verb_row`
  puts it in `cmd.aux` when the line gave none. That pays for one verb: lock, whose aux the parser leaves 0, so the door's
  work and `H_SHUT` each tested the verb (`cmd.action == VB_LOCK_OBJECT ? PA_LOCK : cmd.aux`); both read `cmd.aux` now.
  Skipped where it doesn't pay: the handling verbs (wear is don or wield, take off doff or stow: `handle` reads
  `cmd.aux` whatever the row says), the sway verbs (tell is talk or order, trade six ways), search and examine (their
  aspects).
- **Found, for review:** an overlay's `""` given to `othings` is in the window, and `othings` puts the hub there before
  it copies the name: the name read is the hub's byte at that address (overlay 16's was past the hub's header, in the
  room bank 0 keeps free, where nothing is kept). The timed works moved to overlay 1 give `nil`, an empty name in the main RAM, so
  where they land doesn't matter. resolve.c's `othings (IO_FIND, w, "", 0)` (overlay 11) is the same case, left as it is.
- **Room:** the main RAM 2,117 bytes free to 2,128 (`run16`'s and `run24`'s stubs out, `things_work`'s in). Part 2 1,059
  to 659 (part 24's handlers in, 387 bytes, and the five entries' switches); part 24 3,342 to 3,729; overlay 16 131 to
  1,073; overlay 1 1,288 to 424 (the timed works); overlay 11 307 to 278 (`verb_row`'s request). Overlays 14 (792 to 392)
  and 12 (358 to 745) are part 2's and part 24's banks. Part 2's copy costs some 5,000 cycles more (400 bytes), as a
  command's part is copied back after a call into another part.
- **What's left:** the shared paths stay where they were, not handlers but what they call: main.c's `things`, `take` and
  `took_up` (the world's E_TAKEN's too), `hide`'s work (part 18: its words, the rope's test), `feature` and the
  missiles (part 13), `lift` and `deal` (part 24). The 50 `H_NOT_YET` rows (a verb done is a row and a case in its
  group): none left (grapple's done since 2026-11-09, its own), and no verb's row points at a stub that says "not yet"
  any longer: all 50 done since 2026-11-10 (the powers refused, the rows seen to before the table, status and sneak,
  the three aliases: DECISIONS.md's "The verbs not done yet"; grapple; pray, pray for, fast, swim and follow: its
  "Prayer, fasting, swimming and following"; say, knock, wake, taunt, orate and pretend, minds.c's: DECISIONS.md's
  "Words said, a knock, the minds swayed"; use, push, pull, touch and taste, the things' batch B4's: DECISIONS.md's
  "The things' verbs"; charge, a throw a way, engage, a split attack and again: DECISIONS.md's "Charge, a throw a way,
  engage, a split attack, and again"). The request byte for the handling verbs, if `handle` ever takes its op from the row (the two verbs whose op
  varies would keep theirs in the line). *Done 2026-11-15 (DECISIONS.md's "The dispatch table's rest"): the handling
  verbs whose request is one (put, hang, hold, light, put out, rip) carry it in their rows, a verb carried out as
  another's (use's light, unlock) given none and its row's filled in; wear and take off keep theirs in the line; and
  §4.4's coverage report built (games/dq0/make.js's `coverage`, obj/games/dq0cover.txt: every verb's cell for a thing
  by its word and each part of the place, its handler's, refused by its row, or the handler's own; the sums in the
  build's log). Step L is finished.*

Odd answers the sweeps don't catch (they name the thing, but read oddly), for review: "tie the torch" says "You can't
close the torch." (a tie is a container's, worked as its lid). ("Wear the torch" said "You hold the lit torch in your
left hand already." (a thing for the hand, held): since 2026-11-09 "You can't wear the lit torch.", wear its own verb,
worn only; equip, don and wield as they were: DECISIONS.md's "Wear for what's worn", test dq0wearonly.)

Two decisions recorded in the docs would need revisiting:

- **"The choosers stay two" (DUNGEON.md, 2026-11-04).** Keep two choosers for *minds* (by want, TW_*) versus
  *words*. But naming from words should happen once, and its result (a handle) should travel.
- **The designators (DECISIONS.md, 2026-11-06).** Steps 1–3 above are the designator work done so that it becomes
  the scope table. If the letters are instead attached to today's separate arrays, with the word still sent to the
  world, they become a fifth way of choosing a thing, and the torch bugs come back with letters on them.

---

## 6. In one paragraph

The design documents describe one command pipeline, but the code runs several. There are two readings of each line
(the grammar's and the word scan's, plus the UI re-reading the text). Nameable things have seven kinds of identity.
Four choosers resolve names, and the first one's answer is thrown away. Dispatch is verb-first, so a combination
nobody coded falls through to the verb's no-object version. Items behave consistently because they go through one
store with one chooser. Everything else doesn't. The fix is to make everything nameable behave the way items do:
**one scope table, one resolver, handles not words, an action × class table, refusals as data**, and the grammar,
which already describes the roles, supplying them directly. The designators being built now are the visible face of
that scope table. Built that way, with the stack's id sent rather than its word, they are the first half of the
redesign rather than one more naming mechanism.

---

## 7. The probes

Seed 3, `hmchar -q -s 5 -u feudal -c hunter`, `dq0 -p t -s 3`, the first game with `-k sack`. The probe script plays
a fixed list of lines through `sim/test.js`'s `runTest` (it's modelled on sim/tools/soak.js's one-game runner).

- Game 1: look/examine/push/touch/knock/climb/use/unlock/kick/search/light/put against "the door behind me on my
  right", the torch, the backpack and the sack. It ended at `open it`, when the door's scything blade killed the
  hunter.
- Game 2: the same verbs against the plain door, the iron pot, the well, the sconce, the stairs and the wall, then
  "it" on the pot.
