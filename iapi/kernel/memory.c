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

      ATOMIC_SET (&page_blocks[block]->ref[offset], 0);
      
      return ptr;
    }
    return HWNULL;
}

static void
memory_release(struct iapi_kernel_memory *memory, hwpointer addr)
{
  struct iapi_kernel_memory_page_stack_node *node;

  do
    {
      ATOMIC_STACK_POP (memory->free_stack, node);
    }
  while (node == NULL);

  node->addr = addr;
  ATOMIC_STACK_PUSH (memory->free_pages, node);
}

static size_t
memory_get_page_size (struct iapi_kernel_memory_pagedir *self)
{
  return IAPI_KERNEL_MEMORY_PAGE_SIZE;
}

ATOMIC_STACK (free_cache_stack, char padding);
struct free_cache_stack stack;
static volatile unsigned int size;

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
	      free (cache_node);
	      /* unlock */
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
			      memory_free_cache_compare_nodes);
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
	      struct iapi_kernel_memory_free_cache_node *cache_node;
	      
	      if (vm_node->region.addr + vm_node->region.length < where + size)
		{
		  cache_node =
		    malloc (sizeof (struct iapi_kernel_memory_free_cache));
		  cache_node->node.region.addr =
		    vm_node->region.addr + vm_node->region.length;
		  cache_node->node.region.length =
		    (where + size) -
		    (vm_node->region.addr + vm_node->region.length);
		  RB_TREE_INSERT (self->vm_space, &cache_node->node,
				  memory_vm_space_compare_nodes);
		  RB_TREE_INSERT (self->cache, cache_node,
				  memory_free_cache_compare_nodes);
		}

	      
	      vm_node->region.length = (where - vm_node->region.addr);
	      cache_node = (struct iapi_kernel_memory_free_cache_node *)
		((char *)vm_node -
		 offsetof(struct iapi_kernel_memory_free_cache_node, node));
	      RB_TREE_REMOVE (self->cache, memory_free_cache_rcompare_nodes,
			      cache_node);
	      RB_TREE_INSERT (self->cache, cache_node,
			      memory_free_cache_compare_nodes);
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
	  free (cache_node);
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
			      memory_free_cache_compare_nodes);

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

extern void kernel_map (struct kernel_page_directory *directory,
			hwpointer page, vpointer to);
extern hwpointer kernel_lookup (struct kernel_page_directory *directory,
				vpointer page);
extern void kernel_unmap (struct kernel_page_directory *directory,
			  hwpointer page, vpointer from);

static void 
memory_map(struct iapi_kernel_memory_pagedir *self,
	   hwpointer page, vpointer to)
{
  int block;
  int offset;
  
  block = page/PAGE_BLOCK_SIZE;
  offset = page % PAGE_BLOCK_SIZE;
  ATOMIC_INC (&page_blocks[block]->ref[offset]);
  
  kernel_map (self->directory, page, to);
}

void
memory_unmap (struct iapi_kernel_memory_pagedir *self,
	      vpointer page, size_t size)
{
  int ref;
  int block;
  int offset;

  block = page/PAGE_BLOCK_SIZE;
  offset = page % PAGE_BLOCK_SIZE;
  ATOMIC_XADD (&page_blocks[block]->ref[offset], -1, ref);
  /* TODO: Lookup */
  kernel_unmap (self->directory, kernel_lookup (self->directory, page), page);
  
  if (!ref)
    {
      
    }
}

hwpointer
memory_lookup (struct iapi_kernel_memory_pagedir *self, vpointer page)
{
  return kernel_lookup (self->directory, page);
}

struct iapi_kernel_memory _iapi_kernel_memory_main_memory;
struct iapi_kernel_memory_pagedir _iapi_kernel_memory_kernel_pagedir;

struct iapi_kernel_memory *
iapi_kernel_memory_get_instance ()
{
  return &_iapi_kernel_memory_main_memory;
}

struct iapi_kernel_memory_pagedir *
iapi_kernel_memory_get_kernel_pagedir ()
{
  return &_iapi_kernel_memory_kernel_pagedir;
}

int
liballoc_lock ()
{
  /* lock */
  return 0;
}

int
liballoc_unlock ()
{
  /* unlock */
  return 0;
}

void *
liballoc_alloc(size_t pages)
{
  vpointer addr;
  struct iapi_kernel_memory *mem;
  struct iapi_kernel_memory_pagedir *pagedir;
  size_t iter;
  
  mem = &_iapi_kernel_memory_main_memory;
  pagedir = &_iapi_kernel_memory_kernel_pagedir;
  
  addr = pagedir->reserve (pagedir, NULL,
			   pages * IAPI_KERNEL_MEMORY_PAGE_SIZE); 

  iter = 0;
  while (iter < pages)
    {
      pagedir->map(pagedir, mem->allocate (mem),
		   addr + iter * IAPI_KERNEL_MEMORY_PAGE_SIZE);
    }

  return (void *)addr;
}

int
liballoc_free(void *addr, size_t pages)
{
  struct iapi_kernel_memory *mem;
  struct iapi_kernel_memory_pagedir *pagedir;
  size_t iter;

  mem = &_iapi_kernel_memory_main_memory;
  pagedir = &_iapi_kernel_memory_kernel_pagedir;

  pagedir->unmap (pagedir,
		  (vpointer)addr + iter * IAPI_KERNEL_MEMORY_PAGE_SIZE,
		  pages);

  return 0;
}
