/* cmdmatch.h - Dingus Quest Zero's command matcher (games/dq0c/cmdmatch.s, 65C02 assembly) for C: does a line parse
   by the command grammar? It reads the tables sim/tools/commandtables.js writes: the grammar, its index, and the
   default aliases. Its calls through cmdmatch_c.s, for the tests' harnesses; the game's parser (dq0c) is a task of its
   own, asked through the hub (hub.h's cq). */
#ifndef CMDMATCH_H
#define CMDMATCH_H

/* Set the tables: the grammar (cmd_table.h's bytes), its index (cmd_index.h's) and the default aliases (cmd_alias.h's).
   They stay where they are, and are read in place. Returns 0 if one is not a dq0 table, or they don't belong
   together, or one is too large for the matcher. */
int __fastcall__ cmd_init(const unsigned char *grammar, const unsigned char *index, const unsigned char *aliases);

/* Set the world's words, when they change: space-separated lists of the words the world names things by, the names
   given to things, and the deities' names. The lists must stay as they are until the next cmd_world. */
void __fastcall__ cmd_world(const char *world, const char *names, const char *deities);

/* Match a line. Returns 1 if the line parses, 0 if not, -1 if it is too long or nests too deep for the matcher. */
int __fastcall__ cmd_match(const char *line);

#endif
