#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <atomic_linkedlist.h>
#include <tree.h>

static int
memory_vm_space_compare (struct api_kernel_memory_vm_space_node *node,
			 vpointer addr)
{
  return (addr - node->region.addr);
}

static int
memory_vm_space_compare_nodes (struct api_kernel_memory_vm_space_node *a,
			       struct api_kernel_memory_vm_space_node *b)
{
  return (a->region.addr - b->region.addr);
}

static int
memory_free_cache_compare (struct iapi_kernel_memory_free_cache_node *node,
			   size_t length)
{
  return (length - node->node.region.length);
}

static int
memory_free_cache_compare_nodes (struct iapi_kernel_memory_free_cache_node *a,
				 struct iapi_kernel_memory_free_cache_node *b)
{
  return (a->node.region.length - b->node.region.length);
}

#define PAGE_BLOCK_SIZE (IAPI_KERNEL_MEMORY_PAGE_SIZE/sizeof(short))

struct page_block
{
  volatile unsigned short ref[PAGE_BLOCK_SIZE];
};

static struct page_block **page_blocks;

static hwpointer
memory_allocate (struct iapi_kernel_memory_impl *memory)
{
  struct free_pages_node *node;

  ATOMIC_STACK_POP (memory->free_pages, node);

  if (node != NULL)
    {
      hwpointer ptr;
      int block;
      int offset;
      
      ptr = node->page;
      node->page = HWINV;
      
      ATOMIC_STACK_PUSH (memory->free_stack, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&memory->page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

extern void
kernel_iapi_kernel_memory_map(struct kernel_page_directory *, hwaddress addr,
			      vpointer where);


static size_t
memory_get_page_size (struct iapi_kernel_memory_pageset *self)
{
  return 0x1000;
}

static vpointer
memory_reserve (struct iapi_kernel_memory_pageset *self,
		vpointer where, size_t size)
{
  /* lock */
  if (where)
    {
      struct iapi_kernel_memory_vm_space *vm_node;

      if (RB_TREE_LOOKUP (self->vm_space, &vm_node,
			  memory_vm_space_compare, where))
	{
	  if (vm_node>region->length < size)
	    {
	      /* unlock */
	      return NULL;
	    }
	  else if (vm_node>region->length == size)
	    {
	      RB_TREE_DELETE (vm_node);
	      
	    }
	}
      else
	{

	}
    }
  else
    {

    }
  /* unlock */
}
