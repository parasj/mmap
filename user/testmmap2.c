#include <inc/lib.h>

#define DEBUG 1

void
umain(int argc, char **argv)
{
  // mmap(void *addr, size_t len, int prot, int flags, int fd_num, off_t off)
  struct Fd *fd;
  int r_open;

  size_t length;
  void *mmaped_addr;

  char *content;
  char fread_buf[512];

  // First, open file 'lorem' and get the file id.
  if ((r_open = open("/lorembig", O_RDONLY)) < 0)
    panic("mmap(): opening file failed, ERROR CODE: %d \n", r_open);

  // Start testing.
  cprintf("\nTest mmaping\n");
  length=PGSIZE * 4;
  mmaped_addr = mmap(NULL, length, 0, MAP_PRIVATE, r_open, (off_t) 0);

  content = (char*) mmaped_addr;

  // Read from second page first to test dynamic loading
  cprintf("=> Read from mmapped (post) region:\n%30s\n", (char*)(mmaped_addr + PGSIZE * 4 + 1));
  cprintf("=> Read from mmapped region:\n%30s\n", content);
  munmap(mmaped_addr, length);
}
