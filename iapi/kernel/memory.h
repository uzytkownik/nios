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

enum iapi_kernel_memory_access_flags
  {
    IAPI_KERNEL_MEMORY_ACCESS_SYSTEM = 0 << 0,
    IAPI_KERNEL_MEMORY_ACCESS_READ = 1 << 0,
    IAPI_KERNEL_MEMORY_ACCESS_WRITE = 1 << 1,
    IAPI_KERNEL_MEMORY_ACCESS_EXECUTE = 1 << 2,
    IAPI_KERNEL_MEMORY_ACCESS_COPY_ON_WRITE = 1 << 3
  };

struct iapi_kernel_memory_pagedir
{
  struct iapi iapi;
  size_t (*get_page_size)(struct iapi_kernel_memory_pagedir *self);
  vpointer (*reserve)(struct iapi_kernel_memory_pagedir *self,
		      vpointer where, size_t size);
  void (*map)(struct iapi_kernel_memory_pagedir *self,
	      hwpointer page, vpointer to,
	      enum iapi_kernel_memory_access_flags perm);
  void (*remap)(struct iapi_kernel_memory_pagedir *self,
		vpointer page, size_t size,
		enum iapi_kernel_memory_access_flags perm);
  void (*unmap)(struct iapi_kernel_memory_pagedir *self,
		vpointer page, size_t size);
  hwpointer (*lookup)(struct iapi_kernel_memory_pagedir *self, vpointer page);
  struct kernel_page_directory *directory;
  struct iapi_kernel_memory_vm_space vm_space;
  struct iapi_kernel_memory_free_cache cache;
};

void iapi_kernel_memory_init (void *data);
struct iapi_kernel_memory *iapi_kernel_memory_get_instance ();
struct iapi_kernel_memory_pagedir *iapi_kernel_memory_get_kernel_pagedir ();
void iapi_kernel_memory_pagedir_init (struct iapi_kernel_memory_pagedir *);

#endif
