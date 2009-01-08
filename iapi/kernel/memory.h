#ifndef IAPI_KERNEL_MEMORY
#define IAPI_KERNEL_MEMORY

#include <iapi/iapi.h>
#include <pointers.h>
#include <atomic_linkedlist.h>
#include <tree.h>
#include <liballoc.h>

#define IAPI_KERNEL_MEMORY_PAGE_SIZE 4096
#define IAPI_KERNEL_BLOCK_PAGE_SIZE IAPI_KERNEL_MEMORY_PAGE_SIZE/sizeof(short)

struct iapi_kernel_memory_pagedir;

ATOMIC_STACK (iapi_kernel_memory_page_stack, hwpointer addr);

struct iapi_kernel_memory
{
  struct iapi iapi;
  hwpointer (*allocate)(struct iapi_kernel_memory *memory);
  struct iapi_kernel_memory_pagedir *(*create_pagedir)(struct iapi_kernel_memory *self);
  struct iapi_kernel_memory_page_stack free_pages;
  struct iapi_kernel_memory_page_stack free_stack;
};

struct iapi_kernel_memory_hwregion
{
  hwpointer addr;
  size_t length;
};

struct iapi_kernel_memory_region
{
  vpointer addr;
  size_t length;
};

RB_TREE (iapi_kernel_memory_vm_space,
	 struct iapi_kernel_memory_region region);
RB_TREE (iapi_kernel_memory_free_cache,
	 struct iapi_kernel_memory_vm_space_node node);

struct iapi_kernel_memory_pagedir
{
  struct iapi iapi;
  size_t (*get_page_size)(struct iapi_kernel_memory_pagedir *self);
  vpointer (*reserve)(struct iapi_kernel_memory_pagedir *self,
		      vpointer where, size_t size);
  void (*map)(struct iapi_kernel_memory_pagedir *self,
	      hwpointer page, vpointer to);
  void (*unmap)(struct iapi_kernel_memory_pagedir *self,
		vpointer page, size_t size);
  hwpointer (*lookup)(struct iapi_kernel_memory_pagedir *self, vpointer page);
  struct kernel_page_directory *directory;
  struct iapi_kernel_memory_vm_space vm_space;
  struct iapi_kernel_memory_free_cache cache;
};

void iapi_kernel_memory_init (struct iapi_kernel_memory_hwregion **regions);
extern struct iapi_kernel_memory _iapi_kernel_memory_main_memory;
extern struct iapi_kernel_memory_pagedir _iapi_kernel_memory_kernel_pagedir;
struct iapi_kernel_memory *iapi_kernel_memory_get_instance ();
struct iapi_kernel_memory_pagedir *iapi_kernel_memory_get_kernel_pagedir ();

#endif
