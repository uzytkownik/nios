#include <iapi/kernel/memory.h>
#include <atomic.h>
#include <atomic_linkedlist.h>
#include <tree.h>

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

static int
memory_vm_space_compare (struct iapi_kernel_memory_vm_space_node *node,
			 vpointer addr)
{
  return (addr - node->region.addr);
}

static int
memory_vm_space_compare_nodes (struct iapi_kernel_memory_vm_space_node *a,
			       struct iapi_kernel_memory_vm_space_node *b)
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
  if (a->node.region.length - b->node.region.length)
    return (a->node.region.length - b->node.region.length);
  return (a->node.region.addr - b->node.region.addr);
}

static int
memory_free_cache_rcompare_nodes (struct iapi_kernel_memory_free_cache_node *b,
				  struct iapi_kernel_memory_free_cache_node *a)
{
  if (a->node.region.length - b->node.region.length)
    return (a->node.region.length - b->node.region.length);
  return (a->node.region.addr - b->node.region.addr);
}

#define PAGE_BLOCK_SIZE (IAPI_KERNEL_MEMORY_PAGE_SIZE/sizeof(short))

struct page_block
{
  volatile unsigned short ref[PAGE_BLOCK_SIZE];
};

static struct page_block **page_blocks;

static hwpointer
memory_allocate (struct iapi_kernel_memory *memory)
{
  struct iapi_kernel_memory_page_stack_node *node;

  ATOMIC_STACK_POP (memory->free_pages, node);

  if (node != NULL)
    {
      hwpointer ptr;
      int block;
      int offset;
      
      ptr = node->addr;
      node->addr = HWNULL;
      
      ATOMIC_STACK_PUSH (memory->free_stack, node);

      block = ptr/PAGE_BLOCK_SIZE;
      offset = ptr % PAGE_BLOCK_SIZE;

      ATOMIC_SET (&page_blocks[block]->ref[offset], 1);
      
      return ptr;
    }
    return HWNULL;
}

static size_t
memory_get_page_size (struct iapi_kernel_memory_pageset *self)
{
  return 0x1000;
}

static vpointer
memory_reserve (struct iapi_kernel_memory_pagedir *self,
		vpointer where, size_t size)
{
  /* lock */
  if (where)
    {
      struct iapi_kernel_memory_vm_space_node *vm_node;

      if (RB_TREE_LOOKUP (self->vm_space, &vm_node,
			  memory_vm_space_compare, where))
	{
	  if (vm_node->region.length < size)
	    {
	      /* unlock */
	      return NULL;
	    }
	  else if (vm_node->region.length == size)
	    {
	      struct iapi_kernel_memory_free_cache_node *cache_node;
	      
	      RB_TREE_REMOVE (self->vm_space, memory_vm_space_compare, where);
	      cache_node = (struct iapi_kernel_memory_free_cache_node *)
		((char *)vm_node -
		 offsetof(struct iapi_kernel_memory_free_cache_node, node));
	      RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			      cache_node);
	      /* unlock */
	      free (self->cache);
	      return where;
	    }
	  else
	    {
	      struct iapi_kernel_memory_free_cache_node *cache_node;
	      
	      vm_node->region.addr += size;
	      vm_node->region.length -= size;
	      cache_node = (struct iapi_kernel_memory_free_cache_node *)
		((char *)vm_node -
		 offsetof(struct iapi_kernel_memory_free_cache_node, node));
	      RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			      cache_node);
	      RB_TREE_INSERT (self->cache, cache_node,
			      emory_free_cache_compare_nodes);
	      /* unlock */
	      return where;
	    }
	}
      else if (vm_node)
	{
	  if (vm_node->region.addr > where)
	    vm_node = vm_node->prev;

	  if (vm_node->region.addr + vm_node->region.length > where + size)
	    {
	      /* unlock */
	      return NULL;
	    }
	  else
	    {
	      struct iapi_kernel_memory_free_cache *cache_node;
	      
	      if (vm_node->region.addr + vm_node->region.length < where + size)
		{
		  cache_node =
		    malloc (sizeof (struct iapi_kernel_memory_free_cache));
		  cache_node->node.region->addr =
		    vm_node->region.addr + vm_node->region.length;
		  cache_node->node.region->length =
		    (where + size) -
		    (vm_node->region.addr + vm_node->region.length);
		  RB_TREE_INSERT (self->vm_space, &cache_node->node,
				  memory_vm_space_compare_nodes);
		  RB_TREE_INSERT (self->cache, cache_node,
				  emory_free_cache_compare_nodes);
		}

	      
	      vm_node->region.length = (where - vm_node->region.addr);
	      cache_node = (struct iapi_kernel_memory_free_cache *)
		((char *)vm_node -
		 offsetof(struct iapi_kernel_memory_free_cache, node));
	      RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			      cache_node);
	      RB_TREE_INSERT (self->cache, cache_node,
			      emory_free_cache_compare_nodes);
	      /* unlock */
	      return where;
	    }
	}
      else
	{
	  /* unlock */
	  return NULL;
	}
    }
  else
    {
      struct iapi_kernel_memory_free_cache_node *cache_node;

      if (RB_TREE_LOOKUP (self->cache, &cache_node,
			  memory_free_cache_compare, size))
	{
	  vpointer where;

	  where = cache_node->node.region.addr;
	  RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			  cache_node);
	  RB_TREE_REMOVE (self->vm_space, memory_vm_space_compare, where);
	  /* unlock */

	  return where;
	}
      else
	{
	  RB_TREE_LOOKUP (self->cache, &cache_node, memory_free_cache_compare,
			  (vpointer)(-1));
	  if (cache_node && cache_node->node.region.length > size)
	    {
	      vpointer where;

	      where = cache_node->node.region.addr;
	      cache_node->node.region.addr += size;
	      cache_node->node.region.length -= size;

	      RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			      cache_node);
	      RB_TREE_INSERT (self->cache, cache_node,
			      emory_free_cache_compare_nodes);

	      /* unlock */
	      return where;
	    }
	  else
	    {
	      /* unlock */
	      return NULL;
	    }
	}
    }
}
