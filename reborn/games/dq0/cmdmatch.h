/* cmdmatch.h - the command grammar's matcher: commands.ebnf as the table commandtables.js writes (cmd_table.h).
   The matcher tries every parse of a line (no memo: continuation-passing, so the stack grows with the parse) and
   reports whether the line parses and the fewest adjuncts of any parse, as the host compiler does. */
#ifndef CMDMATCH_H
#define CMDMATCH_H

/* Set the table once: the bytes of cmd_table.h. Returns 0 if the table is not a dq0 grammar table. */
int cmd_init(const unsigned char *table);

/* Match a line. world, names and deities are space-separated lists of the words the world knows.
   Returns 1 if the line parses (*fewest: the fewest adjuncts), 0 if not, -1 if the search ran out of steps. */
int cmd_match(const char *line, const char *world, const char *names, const char *deities, int *fewest);

#endif
