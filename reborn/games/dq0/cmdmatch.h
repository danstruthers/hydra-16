/* cmdmatch.h - the command grammar's matcher: the tables commandtables.js writes (the grammar, and the aliases).
   A returning matcher: each rule's ends from a position are a bit mask, memoised per rule and position. The RAM is
   the memo (rules x positions x 2 bytes, about 6.7 KB) plus the tokens; see cmdmatch.c. */
#ifndef CMDMATCH_H
#define CMDMATCH_H

/* Set the grammar table (the bytes of cmd_table.h). Returns 0 if it is not a dq0 grammar table. */
int cmd_init(const unsigned char *table);

/* Set the default aliases (the bytes of cmd_alias.h). Returns 0 if it is not a dq0 alias table. */
int cmd_init_aliases(const unsigned char *aliases);

/* Match a line. world, names and deities are space-separated lists of the words the world knows.
   Returns 1 if the line parses, 0 if not, -1 if the line is too long. */
int cmd_match(const char *line, const char *world, const char *names, const char *deities, int *fewest);

#endif
