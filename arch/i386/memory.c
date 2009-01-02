#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <atomic_linkedlist.h>
#include <tree.h>

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

RB_TREE(vm_chunk,
//	struct vm_free_list_node *node;
	vpointer addr;
	vpointer length);
//LIST(vm_free_list,
//     struct vm_chunk_node *node);
//RB_TREE(vm_free,
//	vpointer length;
//	struct vm_free_list *list);
struct iapi_kernel_memory_pageset_impl
{
  struct iapi_kernel_memory_pageset parent;
  struct vm_chunk vm_space;
  //struct vm_free free_space
};

static int
kernel_compare(struct vm_chunk_node *node, vpointer addr)
{
  return (addr - node->addr);
}

static int
kernel_compare_nodes(struct vm_chunk_node *a, struct vm_chunk_node *b)
{
  return (a->addr - b->addr);
}

static vpointer
kernel_reserve (struct iapi_kernel_memory_pageset_impl *self,
		vpointer where, unsigned int size)
{
  vpointer res;
  
  res = NULL;
  /* big processor lock + pageset_lock */
  do
    {
      
      if (where != INV)
	{
	  struct vm_chunk_node *nnext;
	  struct vm_chunk_node *iter;
	  
	  
	  RB_TREE_LOOKUP (&self->vm_space, iter, kernel_compare, where);
	  
	  if (iter->addr > where)
	    iter = iter->prev;
	  
	  if (where + size > iter->addr + iter->length)
	    break;

	  if (where + size != iter->addr + iter->length)
	    {
	      //nnext = malloc (...)
	      nnext->addr = where + size;
	      nnext->length =
		(iter->addr + iter->length) - nnext->addr + PAGE_SIZE;
	      RB_TREE_INSERT (&self->vm_space, nnext, kernel_compare_nodes);
	    }

	  iter->length = where - iter->addr;
	}

      
    }
  while (0);
  /* big processor lock + pageset_lock */
  return res;
}
