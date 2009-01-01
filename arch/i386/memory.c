#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <linkedlist.h>

#define PAGE_SIZE 4096
#define PAGE_BLOCK_SIZE (PAGE_SIZE/sizeof(volatile unsigned short))

ATOMIC_STACK(free_pages, hwpointer page);

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

  ATOMIC_STACK_POP (free_pages, node);

  if (node != NULL)
    {
      hwpointer ptr;
      int block;
      int offset;
      
      ptr = node->page;
      node->page = HWINV;
      
      ATOMIC_STACK_PUSH (free_pages_aval, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

