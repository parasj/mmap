// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW         0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
  void *addr = (void*)utf->utf_fault_va;
  uint32_t err = utf->utf_err;
  int r;

  // Check that the faulting access was (1) a write, and (2) to a
  // copy-on-write page.  If not, panic.
  // Hint:
  //   Use the read-only page table mappings at uvpt
  //   (see <inc/memlayout.h>).

  // LAB 4: Your code here.
  if (!(err & FEC_WR))
	  panic("pagefault at [%08x]. Not writing.", addr);

	uint32_t pgnum = (uint32_t) ROUNDDOWN(addr, PGSIZE)/ PGSIZE;
	if (!(uvpt[pgnum] & PTE_COW))
		panic("pgfault: fault was not on a copy-on-write page\n");


  // Allocate a new page, map it at a temporary location (PFTEMP),
  // copy the data from the old page to the new page, then move the new
  // page to the old page's address.
  // Hint:
  //   You should make three system calls.
  // LAB 4: Your code here.
  // Allocate a page
	if ((sys_page_alloc(0, PFTEMP, PTE_U | PTE_W | PTE_P)))
    panic("Failed allocating a page!");

  // Move data from old to new page
	memcpy(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	if (sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_U | PTE_W | PTE_P))
		panic("Failed mapping a page!");

  if (sys_page_unmap(0, PFTEMP))
    panic("Failed to unmap temp page.");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
  int r;

  uint32_t permissions = PTE_P | PTE_U;
  void *addr = (void*)(pn << PGSHIFT);

  if ((uvpt[pn] & (PTE_W | PTE_COW)))
    permissions = permissions | PTE_COW;

  if (sys_page_map(0, addr, envid, addr, permissions) < 0)
    panic("Error mapping %08x", addr);
  if (sys_page_map(0, addr, 0, addr, permissions) < 0)
    panic("Error mapping %08x", addr);
  return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
  // LAB 4: Your code here.
  set_pgfault_handler(pgfault);

  envid_t envid = sys_exofork();

  if (envid < 0)
    panic("Error forking process");

  if (!envid) {
    // We're in the child, fix thisenv
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  for (int i = 0; i < PGNUM(UTOP); i++) {
    // No non-present dirs
    if ((uvpd[(int)PGADDR(0,i,0)] & PTE_P)
        // No non-present pages
        && uvpd[i] & PTE_P
        // No user excpetion stack
        && i << PGSHIFT  != UXSTACKTOP - PGSIZE) {
      duppage(envid, i);
    }
  }

  if (sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P))
    panic("Could not allocate pages.");
  if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall))
    panic("coud not set upcall");
  if (sys_env_set_status(envid, ENV_RUNNABLE))
    panic("could not set runnable");
  return envid;
}

// Challenge!
int
sfork(void)
{
  panic("sfork not implemented");
  return -E_INVAL;
}
