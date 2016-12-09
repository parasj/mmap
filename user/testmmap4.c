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
  cprintf("\nTest mmaping\n");
  length = PGSIZE;
  mmaped_addr = mmap(NULL, length, 0, MAP_PRIVATE, r_open, (off_t) 0);

  content = (char*) mmaped_addr;

  *content = 'z';

  cprintf("=> Read from mmapped region:\n%30s\n", content);
  munmap(mmaped_addr, length);

  // Running cat now on lorem should show a z as the first letter
}
