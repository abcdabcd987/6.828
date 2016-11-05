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
#define TRAPHANDLER(num)									\
	.globl TRAP_HANDLER_FUNC(num);			/* define global symbol for 'name' */	\
	.type TRAP_HANDLER_FUNC(num), @function;	/* symbol type is function */		\
	.align 2;					/* align function definition */		\
	TRAP_HANDLER_FUNC(num):				/* function starts here */		\
	pushl $(num);										\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(num)						\
	.globl TRAP_HANDLER_FUNC(num);					\
	.type TRAP_HANDLER_FUNC(num), @function;			\
	.align 2;							\
	TRAP_HANDLER_FUNC(num):						\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

	TRAPHANDLER_NOEC(T_DIVIDE)
	TRAPHANDLER_NOEC(T_DEBUG)
	TRAPHANDLER_NOEC(T_NMI)
	TRAPHANDLER_NOEC(T_BRKPT)
	TRAPHANDLER_NOEC(T_OFLOW)
	TRAPHANDLER_NOEC(T_BOUND)
	TRAPHANDLER_NOEC(T_ILLOP)
	TRAPHANDLER_NOEC(T_DEVICE)
	TRAPHANDLER(T_DBLFLT)
	// T_COPROC
	TRAPHANDLER(T_TSS)
	TRAPHANDLER(T_SEGNP)
	TRAPHANDLER(T_STACK)
	TRAPHANDLER(T_GPFLT)
	TRAPHANDLER(T_PGFLT)
	// T_RES
	TRAPHANDLER_NOEC(T_FPERR)
	TRAPHANDLER(T_ALIGN)
	TRAPHANDLER_NOEC(T_MCHK)
	TRAPHANDLER_NOEC(T_SIMDERR)

	TRAPHANDLER_NOEC(T_SYSCALL)


/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	push %ds
	push %es
	pushal
	mov $(GD_KD), %ax
	mov %ax, %ds
	mov %ax, %es
	pushl %esp
	call trap
