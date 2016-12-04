
#include <inc/lib.h>
#include <lib/file.c>

void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {

  if (offset) {
    panic("nonzero offsets not supported yet");
  }

  if (flags == MAP_SHARED) {
    panic("Shared maps not supported yet!");
  }

  int numPages = len/PGSIZE;
  void* mapva = (void*)sys_page_save(0, addr, numPages, PTE_SAV);
  int r;

  // Force mappings
  for (int i = 0; i < numPages; i++) {
    if (sys_page_alloc(0, (mapva + PGSIZE * i), PTE_U | PTE_P |PTE_W) < 0) {
      panic("Error allocating file pages");
    }
  }

  // Read entire file into memory
  // read(fd, mapva, len);
  read(fd, mapva, numPages * PGSIZE);

  if (mapva < 0) {
    cprintf("Failed to map page\n");
    return (void*)mapva;
  }

  // strcpy(mapva, "test");
  return mapva;
}
