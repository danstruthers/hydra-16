/* cmdtest.c - the command matcher (games/dq0/cmdmatch.s) on sim65, cc65's 6502 simulator: each corpus command's
   result and cycles, and cmd_init's and cmd_world's, for sim/tools/commandmatch-test.js (the cmd_*.h files are
   commandmatch-gen.js's).  The world is set only when a command's differs from the one before. */
#include <stdio.h>
#include <string.h>
#include <sim65.h>
#include "cmdmatch.h"
#include "cmd_table.h"
#include "cmd_index.h"
#include "cmd_alias.h"
#include "cmd_corpus.h"

static unsigned long clk(void)
{
  peripherals.counter.latch = 0;
  return peripherals.counter.value32[0];
}

static int same_world(int i)
{
  return i > 0 && !strcmp(cmd_case_world[i], cmd_case_world[i - 1]) && !strcmp(cmd_case_names[i], cmd_case_names[i - 1]) &&
    !strcmp(cmd_case_deities[i], cmd_case_deities[i - 1]);
}

int main(void)
{
  int i, r;
  unsigned long t;
  t = clk();
  if (!cmd_init(cmd_table, cmd_index, cmd_alias)) {
    printf("the tables are not right\n");
    return 1;
  }
  printf("init %lu\n", clk() - t);
  for (i = 0; i < CMD_CASES; i++) {
    if (!same_world(i)) {
      t = clk();
      cmd_world(cmd_case_world[i], cmd_case_names[i], cmd_case_deities[i]);
      printf("world %lu\n", clk() - t);
    }
    t = clk();
    r = cmd_match(cmd_text[i]);
    t = clk() - t;
    printf("%d %d %lu\n", i, r, t);
  }
  printf("done %d\n", CMD_CASES);
  return 0;
}
