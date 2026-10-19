/* cmdmatch.h - the command grammar's matcher: the tables commandtables.js writes (the grammar, its index, and the
   default aliases). A returning matcher: each rule's ends from a position are a bit mask, memoised per rule and
   position; words are found by a hash table, and rules and phrases skipped by their first words. See cmdmatch.c. */
#ifndef CMDMATCH_H
#define CMDMATCH_H

/* Set the tables: the grammar (cmd_table.h's bytes), its index (cmd_index.h's) and the default aliases (cmd_alias.h's).
   Returns 0 if one is not a dq0 table, or they don't belong together, or they are too large for the matcher. */
int cmd_init(const unsigned char *grammar, const unsigned char *index, const unsigned char *aliases);

/* Set the world's words, when they change: space-separated lists of the words the world names things by, the names
   given to things, and the deities' names. The lists must stay as they are until the next cmd_world. */
void cmd_world(const char *world, const char *names, const char *deities);

/* Match a line. Returns 1 if the line parses, 0 if not, -1 if the line is too long. */
int cmd_match(const char *line);

#endif
