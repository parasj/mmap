// yield the processor to other environments

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
  int i;

  cprintf("Hello, I am environment %08x.\n", thisenv->env_id);
  for (i = 0; i < 5; i++) {
    sys_yield();
    sys_renice(thisenv->nice + 1);
    cprintf("Back in environment %08x, iteration %d, nice: %d.\n",
            thisenv->env_id, i, thisenv->nice);
  }
  cprintf("All done in environment %08x.\n", thisenv->env_id);
}
