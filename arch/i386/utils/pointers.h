#ifndef ARCH_I386_UTILS_POINTERS_H
#define ARCH_I386_UTILS_POINTERS_H

#define NULL ((vpointer)0)
#define INV ((vpointer)-1)
#define HWNULL 0LL
#define HWINV -1LL

typedef void *vpointer;
typedef unsigned long long hwpointer;

#endif
