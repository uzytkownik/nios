#ifndef IAPI_KERNEL_MEMORY
#define IAPI_KERNEL_MEMORY

#include <iapi/iapi.h>

struct iapi_kernel_memory_pageset;

struct iapi_kernel_memory
{
  struct iapi iapi;
  struct iapi_kernel_memory_pageset *(*create_pageset)(struct iapi_kernel_memory *self);
};

struct iapi_kernel_memory_pageset
{
  struct iapi iapi;
  int (*get_page_size)(struct iapi_kernel_memory_pageset *self);
  void *(*allocate)(struct iapi_kernel_memory_pageset *self, void *where, int size);
  void *(*map)(struct iapi_kernel_memory_pageset *self, void *page, void *to);
  void (*release)(struct iapi_kernel_memory_pageset *self, void *page, int size);
  void *(*lookup)(struct iapi_kernel_memory_pageset *self, void *page);
};

void iapi_kernel_memory_init (int size);
struct iapi_kernel_memory *iapi_kernel_memory_get_instance ();
struct iapi_kernel_memory_pageset *iapi_kernel_memory_get_kernel_pageset ();

#endif
