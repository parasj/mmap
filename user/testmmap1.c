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
  if ((r_open = open("/lorem", O_RDWR)) < 0)
    panic("mmap(): opening file failed, ERROR CODE: %d \n", r_open);

  // Start testing.
  cprintf("\nTest mmaping file as PRIVATE, read from it, and print out the content.\n");
  length = PGSIZE;
  mmaped_addr = mmap(NULL, length, 0, MAP_PRIVATE, r_open, (off_t) 0);

  // // TODO cause this to fail, rather than working
  // cprintf("%s\n", (char*)0xDeadBeef);
  // cprintf("%s\n", (char*)0xCafeBffe);


  content = (char*) mmaped_addr;

  *content = 'z';

  // Read from second page first to test dynamic loading
  // cprintf("=> Read from mmapped region:\n%30s\n", (char*)(mmaped_addr + PGSIZE + 1));
  cprintf("=> Read from mmapped region:\n%30s\n", content);
  munmap(mmaped_addr, length);
}
