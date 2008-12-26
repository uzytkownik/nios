#ifndef ARCH_I386_UTILS_ATOMIC_H
#define ARCH_I386_UTILS_ATOMIC_H

#define ATOMIC_INC(atomic)					\
  __asm__ __volatile__ ("lock; incl %0"				\
			: "=m" (*(atomic))			\
			: "m" (*(atomic)))
#define ATOMIC_DEC(atomic)						\
  __asm__ __volatile__ ("lock; decl %0"					\
			: "=m" (*(atomic))				\
			: "m" (*(atomic)))
#define ATOMIC_ADD(atomic, val)						\
  __asm__ __volatile__ ("lock; addl %1, %0"				\
			: "=m" (*(atomic))				\
			: "m" (*(atomic)),				\
			  "n" (val))
#define ATOMIC_XADD(atomic, val, oval)				\
  __asm__ __volatile__ ("lock; xaddl %0, %1"			\
			: "=r" (oval), "=m" (*(atomic))		\
			: "0" (val), "m" (*(atomic)))
#define ATOMIC_GET(atomic, val) ((void) ((val) = *(atomic)))
#define ATOMIC_SET(atomic, val) ((void) (*(atomic) = (val)))
#define ATOMIC_XCMP(atomic, oval, nval, result)				\
  __asm__ __volatile__ ("lock; cmpxchgl %2, %1"				\
			: "=a" (result), "=m" (*(atomic))		\
			: "r" (nval), "m" (*(atomic)), "0" (oval))

#define ATOMIC_SINC(atomic)					\
  __asm__ __volatile__ ("lock; incw %0"				\
			: "=m" (*(atomic))			\
			: "m" (*(atomic)))
#define ATOMIC_SDEC(atomic)						\
  __asm__ __volatile__ ("lock; decw %0"					\
			: "=m" (*(atomic))				\
			: "m" (*(atomic)))
#define ATOMIC_SADD(atomic, val)					\
  __asm__ __volatile__ ("lock; addw %1, %0"				\
			: "=m" (*(atomic))				\
			: "m" (*(atomic)),				\
			  "n" (val))
#define ATOMIC_SXADD(atomic, val, oval)				\
  __asm__ __volatile__ ("lock; xaddw %0, %1"			\
			: "=r" (oval), "=m" (*(atomic))		\
			: "0" (val), "m" (*(atomic)))
#define ATOMIC_SGET(atomic, val) ((void) ((val) = *(atomic)))
#define ATOMIC_SSET(atomic, val) ((void) (*(atomic) = (val)))
  
#endif
