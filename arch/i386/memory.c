#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <linkedlist.h>

#define PAGE_SIZE 4096
#define PAGE_BLOCK_SIZE (PAGE_SIZE/sizeof(volatile unsigned short))

STACK(free_pages, hwpointer page);

struct page_block
{
  volatile unsigned short ref[PAGE_BLOCK_SIZE];
};

static struct free_pages   free_pages;
static struct free_pages   free_pages_aval;
static struct page_block **page_blocks;

hwpointer
kernel_memory_allocate (struct iapi_kernel_memory *memory)
{
  struct free_pages_node *node;

  STACK_POP (free_pages, node);

  if (node != NULL)
    {
      hwpointer ptr;
      int block;
      int offset;
      
      ptr = node->page;
      node->page = HWINV;
      
      STACK_PUSH (free_pages_aval, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

void
iapi_kernel_memory_init (unsigned int size,
			 hwpointer *usable,
			 unsigned int *usable_lengths)
{
  unsigned int free_pages_n;
  unsigned int page_blocks_n;
  unsigned int page_blocks_assign;
  hwpointer *hiter;
  hwpointer piter;
  unsigned int fpiter;
  hwpointer first_free_page;
  
  free_pages_n = size*sizeof(struct free_pages_node *)/(PAGE_SIZE*PAGE_SIZE);
  page_blocks_n = size/PAGE_BLOCK_SIZE;

  page_blocks = NULL;
  
  hiter = usable;
  first_free_page = 0;
  page_blocks_assign = 0;
  while (*usable_lengths)
    {
      hwpointer page;

      page = *usable;
      while (page < *usable + *usable_lengths)
	{
	  if (free_pages_n != 0)
	    {
	      unsigned int iter;

	      iter = 0;
	      while (iter < PAGE_SIZE/sizeof(struct free_pages_node *))
		{
		  struct free_pages_node *node;

		  node = (void *)(page + sizeof(struct free_pages_node *)*iter);

		  node->next = free_pages_aval.head;
		  free_pages_aval.head = node;
		  
		  node->page = HWINV;		  
		  
		  iter++;
		}
	      
	      free_pages_n--;
	    }
	  else if (page_blocks == NULL)
	    {
	      unsigned int iter;
	      
	      page_blocks = (vpointer)page;

	      iter = 0;
	      while (iter < PAGE_SIZE/sizeof(struct page_block *))
		{
		  page_blocks[iter] = NULL;
		  iter++;
		}	      
	    }
	  else if (page_blocks_n != 0)
	    {
	      page_blocks[page_blocks_assign] = page;
	      page_blocks_n--;
	    }
	  else
	    {
	      struct free_pages_node *node;
	      
	      if (first_free_page == 0)
		{
		  first_free_page = page;
		}

	      node = free_pages_aval.head;
	      free_pages_aval.head = node->next;

	      node->page = page;
	    }

	  page += PAGE_SIZE;
	}
      
      usable++;
      usable_lengths++;
    }

  piter = 0;
  while (piter < size)
    {
      int block;
      int off;

      block = piter / PAGE_BLOCK_SIZE;
      off = piter % PAGE_BLOCK_SIZE;
      
      if (piter < first_free_page)
	{
	  page_blocks[block]->ref[off] = 1;
	}
      else
	{
	  page_blocks[block]->ref[off] = 0;
	}
      piter += PAGE_SIZE;
    }
}
