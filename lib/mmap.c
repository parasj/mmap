
#include <inc/lib.h>
#include <lib/file.c>


volatile struct map_dat mmap_data[MAX_MAPS] = {0};

void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {

  int entry, i, r;
  if (offset) {
    panic("nonzero offsets not supported yet");
  }

  if (flags == MAP_SHARED) {
    panic("Shared maps not supported yet!");
  }

  int numPages = ROUNDUP(len, PGSIZE)/PGSIZE;
  void* mapva = (void*)sys_page_save(0, addr, numPages, PTE_SAV);

  for (i = 0; i < MAX_MAPS; i++) {
    if (!mmap_data[i].present) {
      mmap_data[i].present = true;
      mmap_data[i].fd = fd;
      mmap_data[i].addr = mapva;
      mmap_data[i].len = numPages * PGSIZE;
      break;
    }
  }
  // check if we didn't get an entry
  if (i == MAX_MAPS) {
    cprintf("No maps remaining!\n");
    return NULL;
  }

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

int munmap(void *addr, size_t length) {
  int index = -1;
  for (int i = 0; i < MAX_MAPS; i++) {
    if (mmap_data[i].present && mmap_data[i].addr == addr) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    panic("Tried to unmap nonexisting mapping!");
  }


  for(int i = 0; i < mmap_data[index].len / PGSIZE; i++) {
    void* toUnmap = mmap_data[index].addr + PGSIZE * i;

    // Write back before unmapping
    // seek(mmap_data[index].fd, 0);
    // write(mmap_data[index].fd, toUnmap, PGSIZE);

    // Unmap
    sys_page_unmap(0, toUnmap);
  }

  mmap_data[index].present = false;

  return 0;
}
