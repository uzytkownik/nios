#include <iapi/kernel/memory.h>
#include "../multiboot.h"

struct kernel_page_directory
{
  struct kernel_page_dir *dir;
  hwpointer hwaddress;
};
  
struct kernel_page_dir
{
  unsigned long page_directory[1024];
  struct kernel_page_table *page_table[1024];
};

struct kernel_page_table
{
  unsigned long page_table[1024];
  int padding[1024];
};

static struct iapi_kernel_memory main;
static struct iapi_kernel_memory dma;
static struct iapi_kernel_memory_pagedir kernel_pagedir;

static struct kernel_page_dir kernel_dir;
static struct kernel_page_directory kernel_directory;
static struct kernel_page_table kernel_pagetable;

static inline void
kernel_memory_init (struct iapi_kernel_memory_pagedir *directory)
{
  hwpointer dir[2];
  size_t iter;
  
  dir[0] = main.allocate (&main);
  dir[1] = main.allocate (&main);
  
  directory->directory = malloc (sizeof (struct kernel_page_directory));
  directory->directory->dir =
    (void *)kernel_pagedir.reserve (&kernel_pagedir, NULL, 2*0x1000);
  directory->directory->hwaddress = dir[0];
  kernel_pagedir.map (&kernel_pagedir, dir[0],
		      (vpointer)directory->directory->dir,
		      IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);
  kernel_pagedir.map (&kernel_pagedir, dir[1],
		      (vpointer)directory->directory->dir + 0x1000,
		      IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);

  iter = 0;
  while (iter < 768)
    {
      directory->directory->dir->page_directory[iter] = 0;
      directory->directory->dir->page_table[iter] = NULL;
    }
    
  iter = 0;
  while (iter < 256)
    {
      directory->directory->dir->page_directory[768 + iter]
	= kernel_pagedir.directory->dir->page_directory[iter];
      directory->directory->dir->page_table[768 + iter]
	= kernel_pagedir.directory->dir->page_table[iter];
    }
}

static inline void
kernel_memory_free (struct iapi_kernel_memory_pagedir *directory)
{
  
}

static inline struct iapi_kernel_memory *
kernel_memory_lookup_pagedir (hwpointer ptr)
{
  if (ptr < 0x01000000)
    return &dma;
  return &main;
}

static inline void
kernel_memory_map (struct kernel_page_directory *directory,
		   hwpointer page, vpointer to,
		   enum iapi_kernel_memory_access_flags perm)
{
  struct kernel_page_table *table;
  
  table = directory->dir->page_table[to & 0xffc00000 >> 22];

  if (table == NULL)
    {
      hwpointer hwtable[2];
      struct iapi_kernel_memory_pagedir *pagedir;

      hwtable[0] = main.allocate (&main);
      hwtable[1] = main.allocate (&main);

      table = directory->dir->page_table[to & 0xffc00000 >> 22] =
	(void *)kernel_pagedir.reserve (pagedir, NULL, 2*0x1000);
      if (to < 0xC0000000)
	directory->dir->page_directory[to & 0xffc00000 >> 22] =
	  hwtable[0] | 0x008;
      else
	directory->dir->page_directory[to & 0xffc00000 >> 22] =
	  hwtable[0] | 0x103;
      pagedir->map (pagedir, hwtable[0],
		    (vpointer)directory->dir->page_table[to & 0xffc00000 >> 22],
		    IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);
      pagedir->map (pagedir, hwtable[1],
		    (vpointer)directory->dir->page_table[to & 0xffc00000 >> 22] + 0x1000,
		    IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);
    }

  if (to >= 0xC0000000)
    table->page_table[to & 0x0003ff000 >> 12] = page | (1 << 8);
  
  if (perm & IAPI_KERNEL_MEMORY_ACCESS_READ)
    table->page_table[(vpointer)to & 0x0003ff000 >> 12] |= (1 << 3);
  if ((perm & IAPI_KERNEL_MEMORY_ACCESS_WRITE) &&
      !(perm & IAPI_KERNEL_MEMORY_ACCESS_COPY_ON_WRITE))
    table->page_table[(vpointer)to & 0x0003ff000 >> 12] |= (1 << 2);
}

static inline void
kernel_memory_remap (struct kernel_page_directory *directory,
		     vpointer page, size_t size,
		     enum iapi_kernel_memory_access_flags perm)
{
  struct kernel_page_table *table;

  table = directory->dir->page_table[page & 0xffc00000 >> 22];
  if (perm & IAPI_KERNEL_MEMORY_ACCESS_READ)
    table->page_table[page & 0x0003ff000 >> 12] |= (1 << 3);
  else
    table->page_table[page & 0x0003ff000 >> 12] &= ~(1 << 3);
  if (perm & IAPI_KERNEL_MEMORY_ACCESS_WRITE &&
      !(perm & IAPI_KERNEL_MEMORY_ACCESS_COPY_ON_WRITE))
    table->page_table[page & 0x0003ff000 >> 12] |= (1 << 2);
  else
    table->page_table[page & 0x0003ff000 >> 12] &= ~(1 << 2);
  __asm__ __volatile__ ("invlpg %0"
			: 
			: "m"(*((int *)page)));
}

static inline void
kernel_memory_unmap (struct kernel_page_directory *directory, vpointer page)
{
  struct kernel_page_table *table;

  table = directory->dir->page_table[page & 0xffc00000 >> 22];
  table->page_table[page & 0x0003ff000 >> 12] = 0;
  __asm__ __volatile__ ("invlpg %0"
			:
			: "m"(*((int *)page)));
}

static inline hwpointer
kernel_memory_lookup (struct kernel_page_directory *directory,
		      vpointer page)
{
  struct kernel_page_table *table;
  table = directory->dir->page_table[page & 0xffc00000 >> 22];

  if (table)
    return table->page_table[page & 0x0003ff000 >> 12] & 0x0003ff000;
  
  return HWNULL;
}

struct iapi_kernel_memory *
iapi_kernel_memory_get_instance ()
{
  return &main;
}

struct iapi_kernel_memory_pagedir *
iapi_kernel_memory_get_kernel_pagedir ()
{
  return &kernel_pagedir;
}

static volatile unsigned short page_blocks[0x100000];

#include <arch/common/kernel/memory.c>

#define INIT_PAGE_TABLES 2
static struct kernel_page_table kernel_page_table_init[INIT_PAGE_TABLES];
extern vpointer phys_end;
extern vpointer end;
extern vpointer virt;

static int mem_init = 0;

int
liballoc_lock ()
{
  if (mem_init)
    memory_lock ();
}

int
liballoc_unlock ()
{
  if (mem_init)
    memory_unlock ();
}

void *
liballoc_alloc (size_t pages)
{
  if (mem_init)
    return memory_alloc (pages);
  mem_init = pages;
  return (void *)0xC1000000;
}

int
liballoc_free (void *addr, size_t pages)
{
  return memory_free (addr, pages);
}

void
iapi_kernel_memory_init (void *data)
{
  size_t iter;
  struct iapi_kernel_memory_free_cache_node *free_cache_node;
  
  /* The first call simply initialize the structs */
  main.free_stack.head = NULL;
  iter = 0;
  while (iter < mem_init || mem_init == 0)
    {
      struct iapi_kernel_memory_page_stack_node *node;
      node = malloc (sizeof (struct iapi_kernel_memory_page_stack_node));
      node->next =
	(struct iapi_kernel_memory_page_stack_node *)main.free_stack.head;
      main.free_stack.head = node;
      iter += 1;
    }
  main.free_pages.head = NULL;

  free_cache_node = malloc (sizeof (struct iapi_kernel_memory_free_cache_node));
  free_cache_node->left = free_cache_node->right =
    free_cache_node->next = free_cache_node->prev = NULL;
  free_cache_node->node.left = free_cache_node->node.right =
    free_cache_node->node.next = free_cache_node->node.prev = NULL;
  free_cache_node->node.region.addr = 0xC1000000 + 0x1000 * mem_init;
  free_cache_node->node.region.length = -free_cache_node->node.region.addr;

  kernel_directory.dir = &kernel_dir;
  kernel_directory.hwaddress = (hwpointer)&kernel_dir - virt;
  
  kernel_pagedir.vm_space.head = &free_cache_node->node;
  kernel_pagedir.cache.head = free_cache_node;
  kernel_pagedir.directory = &kernel_directory;

  iter = 0;
  while (iter < 0x01000000)
    {
      kernel_dir.page_directory[768 + iter / 0x400000] = iter | 0x183;
      kernel_dir.page_table[0x300 + iter / 0x400000] = 0;
      iter += 0x400000;
    }

  kernel_dir.page_directory[0x304] = (unsigned long)&kernel_pagetable - virt;
  kernel_dir.page_table[0x304] = &kernel_pagetable;
  
  iter = 0;
  while (iter < mem_init)
    {
      kernel_pagetable.page_table[iter] = (0x01000000 + iter * 0x1000) | 0x183;
      iter++;
    }
  while (iter < 0x400)
    {
      kernel_pagetable.page_table[iter] = 0;
      iter++;
    }

  
}
