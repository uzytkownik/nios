#include <iapi/kernel/memory.h>

struct kernel_page_directory
{
  struct kernel_page_dir *dir;
  hwpointer hwaddress;
};
  
struct kernel_page_dir
{
  unsigned long page_directory[1024];
  struct kernel_page_table *page_table[1024];
} kernel_page_directory;

struct kernel_page_table
{
  unsigned long page_table[1024];
  int padding[1024];
} kernel_page_table_first;

void
kernel_memory_init (struct iapi_kernel_memory_pagedir *directory)
{
  hwpointer dir[2];
  struct iapi_kernel_memory *kmem;
  struct iapi_kernel_memory_pagedir *pagedir;
  size_t iter;
  
  kmem = &_iapi_kernel_memory_main_memory;
  pagedir = &_iapi_kernel_memory_kernel_pagedir;
  
  dir[0] = kmem->allocate (kmem);
  dir[1] = kmem->allocate (kmem);
  
  directory->directory = malloc (sizeof (struct kernel_page_directory));
  directory->directory->dir =
    (void *)pagedir->reserve (pagedir, NULL, 2*0x1000);
  directory->directory->hwaddress = dir[0];
  pagedir->map (pagedir, dir[0], (vpointer)directory->directory->dir,
		IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);
  pagedir->map (pagedir, dir[1], (vpointer)directory->directory->dir + 0x1000,
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
	= kernel_page_directory.page_directory[iter];
      directory->directory->dir->page_table[768 + iter]
	= kernel_page_directory.page_table[iter];
    }
}

void
kernel_memory_free (struct iapi_kernel_memory_pagedir *directory)
{
  
}

struct iapi_kernel_memory *
kernel_memory_lookup_pagedir (hwpointer ptr)
{

}

void
kernel_memory_map (struct kernel_page_directory *directory,
		   hwpointer page, vpointer to,
		   enum iapi_kernel_memory_access_flags perm)
{
  struct kernel_page_table *table;
  
  table = directory->dir->page_table[to & 0xffc00000 >> 22];

  if (table == NULL)
    {
      hwpointer hwtable[2];
      struct iapi_kernel_memory *kmem;
      struct iapi_kernel_memory_pagedir *pagedir;

      kmem = &_iapi_kernel_memory_main_memory;
      pagedir = &_iapi_kernel_memory_kernel_pagedir;

      hwtable[0] = kmem->allocate (kmem);
      hwtable[1] = kmem->allocate (kmem);

      table = directory->dir->page_table[to & 0xffc00000 >> 22] =
	(void *)pagedir->reserve (pagedir, NULL, 2*0x1000);
      if (to < 0xC0000000)
	directory->dir->page_directory[to & 0xffc00000 >> 22] =
	  hwtable[0] | 0x08;
      else
	directory->dir->page_directory[to & 0xffc00000 >> 22] =
	  hwtable[0] | 0x83;
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

void
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

void
kernel_memory_unmap (struct kernel_page_directory *directory,
		     hwpointer page, vpointer from)
{
  struct kernel_page_table *table;

  table = directory->dir->page_table[page & 0xffc00000 >> 22];
  table->page_table[page & 0x0003ff000 >> 12] = 0;
  __asm__ __volatile__ ("invlpg %0"
			:
			: "m"(*((int *)page)));
}

hwpointer
kernel_memory_lookup (struct kernel_page_directory *directory,
		      vpointer page)
{
  struct kernel_page_table *table;
  table = directory->dir->page_table[page & 0xffc00000 >> 22];

  if (table)
    return table->page_table[page & 0x0003ff000 >> 12] & 0x0003ff000;
  
  return HWNULL;
}
