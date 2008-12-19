#ifndef IAPI_KERNEL_MEMORY
#define IAPI_KERNEL_MEMORY

#include <iapi/iapi.h>

struct iapi_kernel_memory_pageset;

struct iapi_kernel_memory
{
  struct iapi iapi;
  struct iapi_kernel_memory_pageset *(*create_pageset)(struct iapi_kernel_memory *self);
  struct iapi_kernel_memory_pageset *(*kernel_pageset)(struct iapi_kernel_memory *self);
};

struct iapi_kernel_memory_pageset
{
  struct iapi iapi;
  int *(*get_size)();
  void *(*allocate)(int n);
  void  (*free)(void *addr, int n);
};

void iapi_kernel_memory_init (int size, void **reserved);
struct iapi_kernel_memory *iapi_kernel_memory_get_instance ();

#endif
