#ifndef IAPI_KERNEL_MEMORY
#define IAPI_KERNEL_MEMORY

#include <iapi/iapi.h>
#include <pointers.h>

struct iapi_kernel_memory_pageset;

struct iapi_kernel_memory
{
  struct iapi iapi;
  hwaddress (*allocate)(struct iapi_kernel_memory *memory);
  struct iapi_kernel_memory_pageset *(*create_pageset)(struct iapi_kernel_memory *self);
};

struct iapi_kernel_memory_pageset
{
  struct iapi iapi;
  unsigned int (*get_page_size)(struct iapi_kernel_memory_pageset *self);
  vaddress (*reserve)(struct iapi_kernel_memory_pageset *self,
		      vaddress where, unsigned int size);
  void (*map)(struct iapi_kernel_memory_pageset *self,
	      hwaddress page, vaddress to);
  void (*unmap)(struct iapi_kernel_memory_pageset *self, vaddress vaddress);
  hwaddress (*lookup)(struct iapi_kernel_memory_pageset *self, vaddress page);
};

void iapi_kernel_memory_init (void **usable, unsigned int *usable_lengths);
struct iapi_kernel_memory *iapi_kernel_memory_get_instance ();
struct iapi_kernel_memory_pageset *iapi_kernel_memory_get_kernel_pageset ();

#endif
