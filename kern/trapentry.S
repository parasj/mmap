/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)                                          \
        .text;                                                           \
        .globl name;            /* define global symbol for 'name' */   \
        .type name, @function;  /* symbol type is function */           \
        .align 2;               /* align function definition */         \
 name:                   /* function starts here */              \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;								                                          \
        .int name

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)                                     \
        .text;                                                           \
        .globl name;                                                    \
        .type name, @function;                                          \
        .align 2;                                                       \
        name:                                                           \
        pushl $0;                                                       \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;								                                          \
        .int name

.data
  .globl trapentry
trapentry:

.text
/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
  TRAPHANDLER_NOEC(trap0, T_DIVIDE)
  TRAPHANDLER_NOEC(trap1, T_DEBUG)
  TRAPHANDLER_NOEC(trap2, T_NMI)
  TRAPHANDLER_NOEC(trap3, T_BRKPT)
  TRAPHANDLER_NOEC(trap4, T_OFLOW)
  TRAPHANDLER_NOEC(trap5, T_BOUND)
  TRAPHANDLER_NOEC(trap6, T_ILLOP)
  TRAPHANDLER_NOEC(trap7, T_DEVICE)
  TRAPHANDLER(trap8, T_DBLFLT)
  TRAPHANDLER(trap9, 9)
  TRAPHANDLER(trap10, T_TSS)
  TRAPHANDLER(trap11, T_SEGNP)
  TRAPHANDLER(trap12, T_STACK)
  TRAPHANDLER(trap13, T_GPFLT)
  TRAPHANDLER(trap14, T_PGFLT)
  TRAPHANDLER_NOEC(trap15, 15)
  TRAPHANDLER_NOEC(trap16, T_FPERR)
  TRAPHANDLER(trap17, T_ALIGN)
  TRAPHANDLER_NOEC(trap18, T_MCHK)
  TRAPHANDLER_NOEC(trap19, T_SIMDERR)
  TRAPHANDLER_NOEC(trap20, 20) // Virtualization?

  TRAPHANDLER_NOEC(handler21, 21)
  TRAPHANDLER_NOEC(handler22, 22)
  TRAPHANDLER_NOEC(handler23, 23)
  TRAPHANDLER_NOEC(handler24, 24)
  TRAPHANDLER_NOEC(handler25, 25)
  TRAPHANDLER_NOEC(handler26, 26)
  TRAPHANDLER_NOEC(handler27, 27)
  TRAPHANDLER_NOEC(handler28, 28)
  TRAPHANDLER_NOEC(handler29, 29)
  TRAPHANDLER_NOEC(handler30, 30)
  TRAPHANDLER_NOEC(handler31, 31)
  TRAPHANDLER_NOEC(handler32, 32)
  TRAPHANDLER_NOEC(handler33, 33)
  TRAPHANDLER_NOEC(handler34, 34)
  TRAPHANDLER_NOEC(handler35, 35)
  TRAPHANDLER_NOEC(handler36, 36)
  TRAPHANDLER_NOEC(handler37, 37)
  TRAPHANDLER_NOEC(handler38, 38)
  TRAPHANDLER_NOEC(handler39, 39)
  TRAPHANDLER_NOEC(handler40, 40)
  TRAPHANDLER_NOEC(handler41, 41)
  TRAPHANDLER_NOEC(handler42, 42)
  TRAPHANDLER_NOEC(handler43, 43)
  TRAPHANDLER_NOEC(handler44, 44)
  TRAPHANDLER_NOEC(handler45, 45)
  TRAPHANDLER_NOEC(handler46, 46)
  TRAPHANDLER_NOEC(handler47, 47)

  TRAPHANDLER_NOEC(trap48, T_SYSCALL)
  TRAPHANDLER_NOEC(trap500, T_DEFAULT)



/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
  # Building le trap frame
  pushl %ds
  pushl %es
  pushal

  // Move GD_KD into ds and es
  movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es

  pushl %esp
  call trap
  // End
