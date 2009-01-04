#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <atomic_linkedlist.h>
#include <tree.h>
#include <liballoc.h>

#define PAGE_SIZE 4096
#define PAGE_BLOCK_SIZE (PAGE_SIZE/sizeof(volatile unsigned short))

ATOMIC_STACK(free_pages, hwpointer page);

struct page_block
{
  volatile unsigned short ref[PAGE_BLOCK_SIZE];
};

struct iapi_kernel_memory_impl
{
  struct free_pages   free_pages;
  struct free_pages   free_pages_aval;
  struct page_block **page_blocks;
} main_memory;

hwpointer
kernel_memory_allocate (struct iapi_kernel_memory_impl *memory)
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
      
      ATOMIC_STACK_PUSH (memory->free_pages_aval, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&memory->page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

RB_TREE(vm_chunk,
	struct vm_free_list_node *node;
	vpointer addr;
	vpointer length);
LIST(vm_free_list,
     struct vm_chunk_node *node);
RB_TREE(vm_free,
	vpointer length;
	struct vm_free_list list);

struct iapi_kernel_memory_pageset_impl
{
  struct iapi_kernel_memory_pageset parent;
  struct vm_chunk vm_space;
  struct vm_free free_space;
  unsigned vpointer **page_table;
} kernel_pageset;

static int
vm_chunk_compare (struct vm_chunk_node *node, vpointer addr)
{
  return (addr - node->addr);
}

static int
vm_chunk_compare_nodes (struct vm_chunk_node *a, struct vm_chunk_node *b)
{
  return (a->addr - b->addr);
}

static int
vm_free_compare (struct vm_free_node *a, vpointer length)
{
  return (length - a->length);
}

static int
vm_free_compare_nodes (struct vm_free_node *a, struct vm_free_node *b)
{
  return (a->length - b->length);
}

