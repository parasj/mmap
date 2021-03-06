// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

// Please guys
#define printf cprintf

#define CMDBUF_SIZE 80 // enough for one VGA text line

struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "backtrace", "Display a backtrace, listing stackframes", mon_backtrace},
  { "shwmap", "Display the mappings pertaining to a virtual address (or a range).", shwmap},
  { "memchmod", "A chmod for your memory. Voids your warranty. Usage: memchmod (+-){PTE_P, PTE_U, PTE_W, PTE_G} VADDR.", memchmod},
  { "vadump", "Dumps contents of the virtual address range. Takes two arguments.", vadump},
  { "padump", "Dumps contents of the physical address range. Takes two arguments.", padump},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
  int i;

  for (i = 0; i < NCOMMANDS; i++)
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_infokern(int argc, char **argv, struct Trapframe *tf)
{
  extern char _start[], entry[], etext[], edata[], end[];

  cprintf("Special kernel symbols:\n");
  cprintf("  _start                  %08x (phys)\n", _start);
  cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
  cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
  cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
  cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
  cprintf("Kernel executable memory footprint: %dKB\n",
          ROUNDUP(end - entry, 1024) / 1024);
  return 0;
}

// return 0 if valid hex, 1 if invalid
int hexsanitize(char* str) {
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
    str += 2;
  }

  for (;*str; str++) {
    char wd = *str;
    if (!((wd <= 70 && wd >= 65) || (wd >= 48 && wd <=57) || (wd >= 97 && wd <= 122))) {
      return -1;
    }
  }
  return 0;
}

// Convert a hex string to integer. Str must have passed hexsanitize
uint32_t hextoi(char* str) {
  // Skip 0x
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
    str += 2;
  }

  uint32_t val = 0;
  for (;*str; str++) {
    char wd = *str;
    uint32_t currentVal = 0;

    if (wd >= 97 && wd <= 122) {
      wd -= 32;
    }

    if (wd <= 70 && wd >= 65) {
      currentVal = wd - 55;
    } else {
      currentVal = wd - 48;
    }
    val = val << 4;
    val |= currentVal;
  }
  return val;
}

int shwmap(int argc, char **argv, struct Trapframe *tf) {

  if (argc < 2) {
    printf("Please enter a memory address to look for.\n");
    return 1;
  }

  if (hexsanitize(argv[1])) {
    printf("You did not enter valid hex.\n");
    return 1;
  }

  char* toPrint = argv[1];
  uint32_t val = hextoi(toPrint);
  uint32_t limit = val;

  if (argc > 2) {
    if (hexsanitize(argv[2])) {
      printf("You did not enter valid hex.\n");
      return 1;
    }

    if (hextoi(argv[1]) >= hextoi(argv[2])) {
      printf("Please input your smaller number first.\n");
    }
    limit = hextoi(argv[2]);
  }

  for (; val <= limit; val += PGSIZE) {
    pde_t* pt = pgdir_walk(kern_pgdir, (void*) val, false);
    if (pt == NULL) {
      printf("0x%08x: 0x%08x -> NULL\n", val);
      continue;
    }
    physaddr_t padr = PTE_ADDR(*pt);

    printf("0x%08x: 0x%08x -> 0x%08x  PERMS:%s%s%s\n", val, val, padr,
           (*pt) & PTE_P ? " PTE_P" : "",
           (*pt) & PTE_W ? " PTE_W" : "",
           (*pt) & PTE_U ? " PTE_U" : "",
           (*pt) & PTE_G ? " PTE_G" : "");
  }

  return 0;
}

int streq(char* one, char* two) {
  while (*one && *two) {
    if (*one != *two) {
      return false;
    }
    one++;
    two++;
  }
  if (*one || *two) {
    return false;
  }
  return true;
}

int memchmod(int argc, char **argv, struct Trapframe *tf) {
  if (argc < 3) {
    printf("You did not enter enough arguments\n");
    return 1;
  }

  if (hexsanitize(argv[argc-1])) {
    printf("You did not enter valid hex.\n");
    return 1;
  }

  uint32_t vaddr = hextoi(argv[argc-1]);
  pde_t* ptptr = pgdir_walk(kern_pgdir, (void*) vaddr, false);

  if (ptptr == NULL) {
    printf("This virtual address does not have a mapping. Create one before using memchmod.\n");
    return 1;
  }

  // We've got to figure out if we have good memory before we do anything...
  for (int i = 1; i < argc - 1; i++) {
    if ((argv[i][0] != '+' && argv[i][0] != '-')
        || !(streq(argv[i] + 1, "PTE_P")
            || streq(argv[i] + 1, "PTE_W")
            || streq(argv[i] + 1, "PTE_U")
            || streq(argv[i] + 1, "PTE_G"))) {
      printf("Malformed argument found.\n");
      return 1;
    }
  }

  for(int i = 1; i < argc - 1; i++) {
    uint32_t perms = 0;
    switch (argv[i][5]) {
    case 'P':
      if (argv[i][0] == '+') {
        *ptptr |= PTE_P;
      } else {
        *ptptr &= ~PTE_P;
      }
      break;
    case 'W':
      if (argv[i][0] == '+') {
        *ptptr |= PTE_W;
      } else {
        *ptptr &= ~PTE_W;
      }
      break;
    case 'U':
      if (argv[i][0] == '+') {
        *ptptr |= PTE_U;
      } else {
        *ptptr &= ~PTE_U;
      }
      break;
    case 'G':
      if (argv[i][0] == '+') {
        *ptptr |= PTE_G;
      } else {
        *ptptr &= ~PTE_G;
      }
      break;
    default:
      // Don't do anything if we're confused.
      break;
    }
  }

  return 0;
}

int vadump(int argc, char** argv, struct Trapframe *tf) {
  return addrDump(argc, argv, tf, true);
}

int padump(int argc, char** argv, struct Trapframe *tf) {
  return addrDump(argc, argv, tf, false);
}

int addrDump(int argc, char** argv, struct Trapframe *tf, int virtual) {
  // Dumps VA's as seen by the kernel

  if (argc <= 2) {
    printf("Not enough arguments provided\n");
    return 1;
  }
  if (hexsanitize(argv[1]) || hexsanitize(argv[2])) {
    printf("There was an error parsing arguments.\n");
    return 1;
  }

  uintptr_t start = hextoi(argv[1]);
  uintptr_t end = hextoi(argv[2]);

  if (start > end) {
    printf("Your start value should be less than your end value\n");
    return 1;
  }

  // Memory is byte addressable. Round down to the nearest 32 bit int (4 bytes).
  start -= (start % sizeof(uintptr_t));
  end -= (end % sizeof(uintptr_t));

  pde_t* ptptr;
  if (virtual)
    ptptr = pgdir_walk(kern_pgdir, (void*) start, false);
  else
    ptptr = NULL;
  for (; start < end; start += sizeof(uintptr_t)) {
    if (!(start % PGSIZE) && virtual) {
      // We're in a new pgframe, update ptptr
      ptptr = pgdir_walk(kern_pgdir, (void*) start, false);
    }

    // Since paging is enabled, we can just go for it.
    if (ptptr == NULL && virtual) {
      // No Mapping
      printf("%x: NULL\n", start);
    } else {
      if (virtual) {
        // Dump data
        printf("%x: 0x%08x\n", start, *((uint32_t*)start));
      } else {
        // Convert to VA
        if (PGNUM(start) >= npages) {
          printf("%x: Kernel does not have access to this PA.\n");
        } else {
          // Dump data
          printf("%x: 0x%08x\n", start, *((uint32_t*)KADDR(start)));
        }
      }
    }
  }

  return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  cprintf("Stack backtrace:\n");
  int* ebp = (int*) read_ebp();
  int* nextebp = ebp;
  int eip;

  while (nextebp != 0x0) {
    // Print info about stack frame currently used
    ebp = nextebp;
    nextebp = (int*) *nextebp;
    eip = *(ebp + 1);

    cprintf("  ebp %x eip %x args", ebp, eip);

    int count = 0;
    ebp += 1;

    // Print out our 5 'args'!
    while (count < 5) {
      ebp += 1;
      count += 1;
      cprintf(" %08x", *ebp);
    }
    cprintf("\n");

    // print info about the debug symbols
    struct Eipdebuginfo dbug;
    debuginfo_eip(eip, &dbug);
    cprintf("         %s:%d: %.*s+%d\n", dbug.eip_file, dbug.eip_line, dbug.eip_fn_namelen, dbug.eip_fn_name, eip - dbug.eip_fn_addr);
  }
  return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
  int argc;
  char *argv[MAXARGS];
  int i;

  // Parse the command buffer into whitespace-separated arguments
  argc = 0;
  argv[argc] = 0;
  while (1) {
    // gobble whitespace
    while (*buf && strchr(WHITESPACE, *buf))
      *buf++ = 0;
    if (*buf == 0)
      break;

    // save and scan past next arg
    if (argc == MAXARGS-1) {
      cprintf("Too many arguments (max %d)\n", MAXARGS);
      return 0;
    }
    argv[argc++] = buf;
    while (*buf && !strchr(WHITESPACE, *buf))
      buf++;
  }
  argv[argc] = 0;

  // Lookup and invoke the command
  if (argc == 0)
    return 0;
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");

  if (tf != NULL)
    print_trapframe(tf);

  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}
