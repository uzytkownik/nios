#ifndef ARCH_I386_UTILS_ATOMIC_H
#define ARCH_I386_UTILS_ATOMIC_H

#define ATOMIC_INC(atomic) __asm__ __volatile__ ("lock; incl %0"	\
						 : "=m" (*(atomic))	\
						 : "m" (*(atomic)))
#define ATOMIC_DEC(atomic) __asm__ __volatile__ ("lock; decl %0"	\
						 : "=m" (*(atomic))	\
						 : "m" (*(atomic)))
#define ATOMIC_GET(atomic, val) ((void) ((val) = *(atomic)))
#define ATOMIC_SET(atomic, val) ((void) (*(atomic) = (val)))

#endif
