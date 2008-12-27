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

static struct free_pages free_pages;
static struct free_pages free_pages_aval;
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
      node->page = HWNULL;
      
      STACK_PUSH (free_pages_aval, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

/* We can map first 8 MiB without allocation */
#define BOOTUP_BLOCK 1
static struct free_pages_node fpn_start[BOOTUP_BLOCK * PAGE_BLOCK_SIZE];
static struct page_block first_block[BOOTUP_BLOCK];
/*
 * We can map systems without PAE without problems.
 * Please note that the memory will be reserved in BOOTUP_BLOCKS chunks.
 */
#define BOOTUP_BLOCKS_PAGES 1
#define BLOCKS_PER_PAGE (PAGE_SIZE/sizeof(struct page_block *))
#define BOOTUP_BLOCKS (BOOTUP_BLOCKS_PAGES*BLOCKS_PER_PAGE)
static struct page_block *bootup_blocks[BOOTUP_BLOCKS];

/*
 * First phase of memory subsystem initialization
 * The goal is to initalize everything that can be achived without need
 * od dynamic allocation of functions
 */
static unsigned int
free_memory_init_1 (hwpointer start, unsigned int length)
{
  hwpointer pmax;
  unsigned int iter;

  if (length > PAGE_SIZE * BOOTUP_BLOCK * PAGE_BLOCK_SIZE)
    length = PAGE_SIZE * BOOTUP_BLOCK * PAGE_BLOCK_SIZE;

  pmax = start;
  iter = 0;
  while (pmax < start + length)
    {
      fpn_start[iter].page = pmax;
      fpn_start[iter].next = &fpn_start[iter + 1];
      
      iter++;
      pmax += PAGE_SIZE;
    }
  fpn_start[iter - 1].next = 0;
  free_pages.head = &fpn_start[0];

  if (iter < BOOTUP_BLOCK * PAGE_BLOCK_SIZE)
    {
      free_pages_aval.head = &fpn_start[iter];
      while (iter < BOOTUP_BLOCK * PAGE_BLOCK_SIZE)
	{
	  fpn_start[iter].page = HWNULL;
	  fpn_start[iter].next = &fpn_start[iter + 1];
	  iter++;
	}
      fpn_start[BOOTUP_BLOCK * PAGE_BLOCK_SIZE - 1].next = NULL;
    }
  else
    {
      free_pages_aval.head = NULL;
    }

  return length;
}

void
iapi_kernel_memory_init (hwpointer *usable, unsigned int *usable_lengths)
{
  
}
