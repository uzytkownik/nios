#include <iapi/kernel/memory.h>

/*
 * The start of memory is reserved for kernel:
 * - on  <16 MiB systems the top 4MiB
 * - on >=16 MiB systems the top 8MiB
 *
 * The kernel pages are 4 MiB. The first one is reserved for sole kernel use.
 * The kernel stack and code as well as DMA buffers are located there. The
 * modules are located there only at the beginning.
 *
 * The ref counting for the first 2 kernel pages can be located at known
 * location but anywhere. Each other looks contains:
 * +--------+------------------------------------+
 * |   page | description                        |
 * +--------+------------------------------------+
 * |  0x000 | Reference counting for other pages |
 * |  0x001 | Data                               |
 * |   ...  | ...                                |
 * +--------+------------------------------------+
 *
 * == Allocation ==
 * Each free page have structure of
 * +--------+------------------------------------+
 * | Offset | Meaning                            |
 * +--------+------------------------------------+
 * |  0x000 | Next free page (first only)        |
 * |  0x004 | Previous free page (first_only)    |
 * |  0x008 | First free block                   |
 * |  0x00C | Last free block                    |
 * |  0x010 | Reserved                           |
 * +--------+------------------------------------+
 *
 * allocations - vector of areas of pages of given size
 * biggest_allocation - the biggest possibly area to allocate memory
 *
 * === Acquire ===
 * 1. Page of given size or worst fit is taken from vector
 * 2. The unused part is returned
 * 3. The pointers of next/previous vector are updated
 * 4. The ref counting is set to 1
 *
 * === Release ===
 * 1. The ref counting is decreased
 * 2. DONE if non-zero
 * 3. Check if previous area free. If yes - merge
 * 4. Check if next area free. If yes - merge
 */

struct page
{
  struct page *next;
  struct page *prev;
  struct page *begin;
  struct page *finish;
  char padding[4084];
};

struct kernel_page_header
{
  int ref[1024];
};


static struct iapi_kernel_memory i386_memory;
static struct iapi_kernel_memory_pageset kernel_pageset;
static unsigned long kernel_page_directory[1024] __attribute__ ((aligned (4096)));
static unsigned long kernel_page_table[1024] __attribute__ ((aligned (4096)));
static volatile int biggest_allocation;
static struct page *allocations[1024];

static int
kernel_get_page_size (struct iapi_kernel_memory_pageset *self)
{
  return 4*1024;
}

static void *
kernel_allocate (struct iapi_kernel_memory_pageset *self,
		 void *where, int size)
{
  struct page *page;
  struct kernel_page_header *header;
  int bucket;
  int offset;
  int iter;
  
  if (where != (void *)-1)
    return 0;

  if (size > biggest_allocation)
    return 0;

  if (allocations[size] != 0)
    {
      page = allocations[size];
      bucket = size;
    }
  else
    {
      struct page *new_page;
      struct page *old_page;
      int iter;
      
      page = allocations[biggest_allocation];
      bucket = biggest_allocation;
      
      new_page = page + (bucket - size);
      old_page = allocations[bucket - size];

      new_page->next = old_page;
      new_page->prev = 0;

      if (old_page != 0)
	{
	  old_page->prev = new_page;
	}

      allocations[bucket - size] = new_page;
      iter = bucket - size;
      while (iter--)
	{
	  (new_page + iter)->begin = new_page;
	}
    }

  if (page->next != 0)
    {
      page->next->prev = 0;
    }
  allocations[bucket] = page->next;

  header = (struct kernel_page_header *)((int)page & 0xFFC00000);
  offset = ((int)page & 0x003FFFFF)/1024;

  iter = size - 1;
  while (iter--)
    {
      header->ref[offset + iter] = 1;
    }

  return page;
}

static inline void
kernel_mark_free (struct page *start, struct page *finish,
		  struct page *begin, struct page *end,
		  struct kernel_page_header *header)
{
  struct page *old_page;
  int sub_iter;
  int offset;
  
  if (begin < start)
    {
      if (begin->prev == 0)
	{
	  allocations[start - begin] = begin->next;
	}
      else
	{
	  begin->prev->next = begin->next;
	}

      if (begin->next != 0)
	{
	  begin->next->prev = begin->prev;
	}
    }

  if (end < finish)
    {
      if (finish->prev == 0)
	{
	  allocations[finish - end] = finish->next;
	}
      else
	{
	  finish->prev->next = finish->next;
	}

      if (finish->next != 0)
	{
	  finish->next->prev = finish->prev;
	}
    }
  
  old_page = allocations[finish - begin];
  allocations[finish - begin] = begin;
  
  begin->prev = 0;
  begin->next = old_page;
  
  if (old_page != 0)
    {
      old_page->prev = begin;
    }

  offset = ((int)begin & 0x003FFFFF)/1024;
  sub_iter = finish - begin;
  while (sub_iter--)
    {
      (begin + sub_iter)->begin = begin;
      (begin + sub_iter)->finish = finish;
    }
}

static void
kernel_release (struct iapi_kernel_memory_pageset *self, void *_page, int size)
{
  struct page *page;
  struct page *begin;
  struct kernel_page_header *header;
  int offset;
  int iter;

  page = (struct page *)_page;
  header = (struct kernel_page_header *)((int)page & 0xFFC00000);
  offset = ((int)page & 0x003FFFFF)/1024;

  if (offset > 1 && header->ref[offset - 1] == 0)
    {
      begin = (page - 1)->begin;
    }
  else
    {
      begin = 0;
    }
  iter = 0;
  while (iter < size)
    {
      header->ref[offset + iter]--;
      if (begin == 0)
	{
	  if (header->ref[offset + iter] == 0)
	    {
	      begin = page + iter;
	    }
	}
      else
	{
	  if (header->ref[offset + iter] != 0)
	    {
	      kernel_mark_free (page, page + size, begin, page + iter, header);
	      begin = 0;
	    }
	}
      iter++;
    }

  if (begin != 0)
    {
      if (offset + iter == 1024)
	{
	  kernel_mark_free (page, page + size, begin, page + iter, header);
	}
      else
	{
	  kernel_mark_free (page, page + size,
			    begin, (page + iter)->finish, header);
	}
    }
}

static void *
kernel_lookup (struct iapi_kernel_memory_pageset *self, void *page)
{
  /* Kernel is identity mapped */
  return page;
}

static void *
kernel_map (struct iapi_kernel_memory_pageset *self, void *page, void *to)
{
  /* No mapping */
  return 0;
}

void
iapi_kernel_memory_init (int size)
{
  int iter;
  
  iapi_new (&i386_memory.iapi, 0);
  i386_memory.create_pageset = 0;
  
  iapi_new (&kernel_pageset.iapi, 0);
  kernel_pageset.get_page_size = kernel_get_page_size;
  kernel_pageset.allocate = kernel_allocate;
  kernel_pageset.map = kernel_map;
  kernel_pageset.release = kernel_release;
  kernel_pageset.lookup = kernel_lookup;

  kernel_page_directory[0] = (int)&kernel_page_table | 0x08B;
  iter = 0;
  while (iter < 1024)
    {
      kernel_page_directory[iter++] = 0x082;
    }
  
  iter = 0;
  while (iter < size/0x400000)
    {
      kernel_page_table[iter] = (iter * 0x400000) | 0x01B;
      iter++;
    }
  while (iter < 1024)
    {
      kernel_page_table[iter++] = 0x002;
    }

  __asm__ __volatile__ ("movl %0, %%cr3"
			:
			: "q"(kernel_page_directory));
  __asm__ __volatile__ ("movl %%cr3, %%eax\n\t"
			"or $0x80000000, %%eax\n\t"
			"mov %%eax, %%cr3"
			:
			:
			: "%eax");

  if (size >= 0x1000000)
    {
      iter = 2;
    }
  else
    {
      iter = 1;
    }
  allocations[1023] = 0;
  while (iter < size/0x400000)
    {
      struct kernel_page_header * header;
      struct page *begin;
      struct page *finish;
      int subiter;
      
      header = (struct kernel_page_header *)(iter * 0x400000);
      begin = (struct page *)(iter * 0x400000 + 0x1000);
      finish = (struct page *)((iter + 1) * 0x400000 + 0x1000);

      begin->next = allocations[1023];
      if (allocations[1023] != 0)
	{
	  allocations[1023]->prev = begin;
	}
      allocations[1023] = begin;
      
      subiter = 0;
      while (subiter < 1023)
	{
	  header->ref[subiter + 1] = 0;
	  (begin + subiter)->begin = begin;
	  (begin + subiter)->finish = finish;
	  subiter++;
	}
      iter++;
    }
  allocations[1023]->prev = 0;
  biggest_allocation = 1023;
}

struct iapi_kernel_memory *
iapi_kernel_memory_get_instance ()
{
  return &i386_memory;
}

