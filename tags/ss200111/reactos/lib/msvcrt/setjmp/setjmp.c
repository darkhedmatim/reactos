/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* modified by Boudewijn Dekker */
/* ms uses a smaller jmp_buf structure */
/* might do a realloc in setjmp */

typedef struct {
  unsigned int __eax, __ebx, __ecx, __edx, __esi;
  unsigned int __edi, __ebp, __esp, __eip, __eflags;
  unsigned short __cs, __ds, __es, __fs, __gs, __ss;
  unsigned long __sigmask; /* for POSIX signals only */
  unsigned long __signum; /* for expansion */
  unsigned long __exception_ptr; /* pointer to previous exception */
  unsigned char __fpu_state[108]; /* for future use */
} jmp_buf[1];


/* jumps back to position specified in jmp_buf */

int longjmp( jmp_buf env, int value )
{
  //push ebp             generated by the compiler
  //mov ebp, esp

  __asm__ __volatile__ (
	"movl	8(%ebp),%edi\n\t"	/* get jmp_buf */
	"movl	12(%ebp),%eax\n\t"	/* store retval in j->eax */
	"movl	%eax,0(%edi)\n\t"

	"movw	46(%edi),%fs\n\t"
	"movw	48(%edi),%gs\n\t"
	"movl	4(%edi),%ebx\n\t"
	"movl	8(%edi),%ecx\n\t"
	"movl	12(%edi),%edx\n\t"
	"movl	24(%edi),%ebp\n\t"

	/* Now for some uglyness.  The jmp_buf structure may be ABOVE the
	   point on the new SS:ESP we are moving to.  We don't allow overlap,
	   but do force that it always be valid.  We will use ES:ESI for
	   our new stack before swapping to it.  */

	"movw	50(%edi),%es\n\t"
	"movl	28(%edi),%esi\n\t"
	"subl	$28,%esi\n\t"	/* We need 7 working longwords on stack */

	"movl	60(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,(%esi)\n\t"	/* Exception pointer */

	"movzwl	42(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,4(%esi)\n\t"	/* DS */

	"movl	20(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,8(%esi)\n\t"	/* EDI */

	"movl	16(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,12(%esi)\n\t"	/* ESI */

	"movl	32(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,16(%esi)\n\t"	/* EIP - start of IRET frame */

	"movl	40(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,20(%esi)\n\t"	/* CS */

	"movl	36(%edi),%eax\n\t"
	"es\n\t"
	"movl	%eax,24(%esi)\n\t"	/* EFLAGS */

	"movl	0(%edi),%eax\n\t"
	"movw	44(%edi),%es\n\t"

	"movw	50(%edi),%ss\n\t"
	"movl	%esi,%esp\n\t"

	//"popl	___djgpp_exception_state_ptr\n\t"
	"popl	%edi\n\t"	// dummy popl instead of djgpp_exception_state_ptr
	"popl	%ds\n\t"
	"popl	%edi\n\t"
	"popl	%esi\n\t"

	"iret\n\t"		/* actually jump to new cs:eip loading flags */
	);

  return value; // dummy return never reached
}


int _setjmp( jmp_buf env )
{
  //push ebp             generated by the compiler
  //mov ebp, esp

  __asm__ __volatile__ (
	"pushl	%edi\n\t"
	"movl	8(%ebp),%edi\n\t"

	"movl	%eax, (%edi)\n\t"
	"movl	%ebx,4(%edi)\n\t"
	"movl	%ecx,8(%edi)\n\t"
	"movl	%edx,12(%edi)\n\t"
	"movl	%esi,16(%edi)\n\t"

	"movl	-4(%ebp),%eax\n\t"
	"movl	%eax,20(%edi)\n\t"

	"movl	(%ebp),%eax\n\t"
	"movl	%eax,24(%edi)\n\t"

	"movl	%esp,%eax\n\t"
	"addl	$12,%eax\n\t"
	"movl	%eax,28(%edi)\n\t"

	"movl	4(%ebp),%eax\n\t"
	"movl	%eax,32(%edi)\n\t"

	"pushfl\n\t"
	"popl	36(%edi)\n\t"

	"movw	%cs, 40(%edi)\n\t"
	"movw	%ds, 42(%edi)\n\t"
	"movw	%es, 44(%edi)\n\t"
	"movw	%fs, 46(%edi)\n\t"
	"movw	%gs, 48(%edi)\n\t"
	"movw	%ss, 50(%edi)\n\t"

	//movl	___djgpp_exception_state_ptr, %eax
	//movl	%eax, 60(%edi)

	"popl	%edi\n\t"
	);

  return 0;
}
