#ifndef ARCH_I386_UTILS_LINKEDLIST_H
#define ARCH_I386_UTILS_LINKEDLIST_H

#define ATOMIC_STACK(name, data...)			\
  struct name ##_node					\
  {							\
    struct name ## _node *next;				\
    data;						\
  };							\
  struct name						\
  {							\
    volatile struct name ## _node *head;		\
  };

#define ATOMIC_STACK_POP(stack, node)					\
  __asm__ __volatile__ ("\n"						\
			"1:\n"						\
			"\ttest %%eax, %%eax\n"				\
			"\tjz 1f\n"					\
			"\tmov (%%eax), %%ecx\n"			\
			"\tlock cmpxchg %%ecx, %0\n"			\
			"\tjnz 1b\n"					\
			"1:"						\
			: "=m"((stack).head), "=a"(node)		\
			: "m"((stack).head), "a"((stack).head)		\
			: "%ecx");
#define ATOMIC_STACK_PUSH(stack, node)					\
 __asm__ __volatile__ ("\n"						\
		       "1:\n"						\
		       "\tmov %%eax, (%%ebx)\n"				\
		       "\tlock cmpxchg %%ebx, %0\n"			\
		       "\tjnz 1b\t"					\
		       : "=m"((stack).head), "=m"((node)->next)		\
		       : "m" ((stack).head), "b"((node)),		\
		       "a" ((stack).head));			       

#endif
