#include <iapi/kernel/memory.h>
#include <atomic.h>

#pragma pack(1)

#define PAGE_SIZE 4096

struct free_page
{
  struct free_page *next;
  hwpointer page;
};

struct page_block
{
  volatile unsigned short ref[2048];
};

#pragma pack()

volatile struct free_page *fpages;
volatile struct free_page *free_fpages;
struct page_block **page_blocks;

static hwpointer
kernel_memory_allocate (struct iapi_kernel_memory *memory)
{
  /*
   * TODO: Check if it is much slower then in assembler
   */
  struct free_page *page;
  struct free_page *old_page;

  ATOMIC_GET (&fpages, page);
  while (page != NULL)
    {
      struct free_page *next;

      next = page->next;
      ATOMIC_XCMP (&fpages, page, next, old_page);
      if (page == old_page)
	{
	  do
	    {
	      hwpointer ptr;
	      
	      ptr = page->page;
	      page->page = HWNULL;
	      
	      page->next = free_fpages;
	      ATOMIC_XCMP (&free_fpages, page->next, page, old_page);
	      if (page->next == old_page)
		{
		  int block;
		  int off;

		  block = ptr/2048;
		  off = ptr % 2048;

		  ATOMIC_SSET (&page_blocks[block]->ref[off], 1);
		  
		  return ptr;
		}
	    }
	  while (1);
	}
      page = old_page;
    }

  return HWNULL;
}
