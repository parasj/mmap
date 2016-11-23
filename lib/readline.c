#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/string.h>

#define BUFLEN 1024
#define HISTORY 10

static char buf[BUFLEN * HISTORY];
static char* prev = &buf[BUFLEN];

int32_t bufPtr = 0;

char *
readline(const char *prompt)
{
  int i, c, echoing;

#if JOS_KERNEL
  if (prompt != NULL)
    cprintf("%s", prompt);
#else
  if (prompt != NULL)
    fprintf(1, "%s", prompt);
#endif

  echoing = iscons(0);
  bufPtr = 0;

  // Clear out first buffer
  memset(buf, 0, sizeof(buf[0] * BUFLEN));

  while (1) {
    c = getchar();
    if (c < 0) {
      if (c != -E_EOF)
        cprintf("read error: %e\n", c);
      return NULL;
    } else if ((c == '\b' || c == '\x7f') && buf[BUFLEN * bufPtr]) {
      if (echoing)
        cputchar('\b');
      int len = strlen(&buf[BUFLEN * bufPtr]);
      buf[BUFLEN * bufPtr + len - 1] = 0;
    } else if (c >= ' ' && strlen(&buf[BUFLEN * bufPtr]) < BUFLEN-1) {
      if (echoing)
        cputchar(c);
      strcat(&buf[BUFLEN * bufPtr], (const char*)&c);
      // buf[i++] = c;
    } else if (c == '\n' || c == '\r') {
      if (echoing)
        cputchar('\n');

      // Crappy way to use memcpy, but here we are.

      // Since we're committing, first make whatevers in the current buffer our 'most recent entry'
      memcpy(&buf[0], &buf[BUFLEN * bufPtr], sizeof(buf[0]) * BUFLEN);

      for (int j = HISTORY - 1; j > 0; j--) {
        // Clear out target
        memset(&buf[BUFLEN * (j)], 0, sizeof(buf[0] * BUFLEN));
        memcpy(&buf[BUFLEN * j], &buf[BUFLEN * (j - 1)], sizeof(buf[0]) * BUFLEN);
      }

      // buf[BUFLEN * bufPtr ] = 0;
      return &buf[BUFLEN * 1];
    } else if (c == 16) { // ctrl-p
      bufPtr++;
      if (bufPtr > HISTORY - 1) {
        bufPtr = HISTORY - 1;
      }

      // Assume width of terminal is 60. We can afford bugs otherwise ;)
      cprintf("\r%*s \r%s", 60, " ", prompt); // Reset and clear line
      if (prompt != NULL)
        cprintf("%s", &buf[BUFLEN * bufPtr]);

    } else if (c == 14) { // Ctrl-n
      bufPtr--;
      if (bufPtr < 0) {
        bufPtr = 0;
      }
      cprintf("\r%*s \r%s", 60, " ", prompt); // Reset and clear line
      if (prompt != NULL)
        cprintf("%s", &buf[BUFLEN * bufPtr]);
    }
  }
}
