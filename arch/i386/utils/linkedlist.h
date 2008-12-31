#ifndef ARCH_I386_UTILS_LINKEDLIST_H
#define ARCH_I386_UTILS_LINKEDLIST_H

#define STACK(name, data...)			\
  struct name ##_node				\
  {						\
    struct name ## _node *next;			\
    data;					\
  };						\
  struct name					\
  {						\
    volatile struct name ## _node *head;	\
  };

#define STACK_POP(stack, node)						\
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
#define STACK_PUSH(stack, node)						\
 __asm__ __volatile__ ("\n"						\
		       "1:\n"						\
		       "\tmov %%eax, (%%ebx)\n"				\
		       "\tlock cmpxchg %%ebx, %0\n"			\
		       "\tjnz 1b\t"					\
		       : "=m"((stack).head), "=m"((node)->next)		\
		       : "m" ((stack).head), "b"((node)),		\
		       "a" ((stack).head));				\

#define QUEUE(name, data)					\
  struct name ## _node						\
  {								\
    struct name ## _node *next;					\
    data;							\
  };								\
								\
  struct name							\
  {								\
    volatile struct name ## _node *head;			\
    volatile struct name ## _node *tail;			\
  };

#define QUEUE_PUSH(queue, node)						\
  __asm__ __volatile__ ("\n"						\
			"1:\n"						\
			"\tmov %0, %%ebx\n"				\
			"\tmov (%%ebx), %%ecx\n"			\
			"\tcmp %%ebx, %0\n"				\
			"\tjnz 1b\n"					\
			"\ttest %%ecx, %%ecx\n"				\
			"\tjnz 2f\n"					\
			"\txor %%eax, %%eax\n"				\
			"\tlock cmpxchg %%edx, (%%ecx)\n"		\
			"\tjnz 1b\n"					\
			"\tjmp 3f\n"					\
			"2:\n"						\
			"\tmov %%ebx, %%eax\n"				\
			"\tlock cmpxchg %%ecx, %0\n"			\
			"\tjmp 1b\n"					\
			"3:\n"						\
			: "=m"((queue).tail)				\
			: "m"((queue).tail), "d"((node))		\
			: "%eax", "%ebx", "%ecx")

#define QUEUE_POP(queue, head, old_head) \
  __asm__ __volatile__ ("\n"						\
			"1:\n"						\
			"\tmov %0, %%ebx\n"				\
			"\tmov %1, %%ecx\n"				\
			"\tmov (%%ebx), %%edx\n"			\
			"\tcmp %0, %%ebx\n"				\
			"\tjnz 1b\n"					\
			"\tcmp %%ebx, %%ecx"				\
			"\tjnz 2f\n"					\
			"\ttest %%edx, %%edx\n"				\
			"\tjnz 3f\n"					\
			"\txor %%eax, %%eax\n"				\
			"\txor %%edx, %%edx\n"				\
			"\tjmp 4f\n"					\
			"3:"						\
			"\tmov %%ecx, %%eax\n"				\
			"\tlock cmpxchg %%edx, %1\n"			\
			"\tjmp 1b\n"					\
			"2:"						\
			"\tmov %%ebx, %%eax\n"				\
			"\tlock cmpxchg %%edx, %0\n"			\
			"\tjnz 1b\n"					\
			"4:"						\
  			: "=m"((queue).head), "=m"((queue).tail)	\
			  "=a"(old_head), "=d"(head)			\
			: "m"((queue).head), "m"((queue).tail)		\
			: )
    
#endif
