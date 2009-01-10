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

static inline volatile unsigned short *
get_page_block_ref (hwpointer addr)
{
  int block;
  int offset;

  block = block / PAGE_BLOCK_SIZE;
  offset = block % PAGE_BLOCK_SIZE;

  return &page_blocks[block]->ref[offset];
}

static hwpointer
memory_allocate (struct iapi_kernel_memory *memory)
{
  struct iapi_kernel_memory_page_stack_node *node;

  ATOMIC_STACK_POP (memory->free_pages, node);

  if (node != NULL)
    {
      hwpointer ptr;
      
      ptr = node->addr;
      node->addr = HWNULL;
      
      ATOMIC_STACK_PUSH (memory->free_stack, node);
      ATOMIC_SET (get_page_block_ref(ptr), 0);
      
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

extern void kernel_memory_init (struct iapi_kernel_memory_pagedir *);
extern struct kernel_page_directory *kernel_memory_get_kernel ();
extern void kernel_memory_map (struct kernel_page_directory *directory,
			       hwpointer page, vpointer to,
			       enum iapi_kernel_memory_access_flags perm);
extern void kernel_memory_remap (struct kernel_page_directory *directory,
				 vpointer page, size_t size,
				 enum iapi_kernel_memory_access_flags perm);
extern hwpointer kernel_memory_lookup (struct kernel_page_directory *directory,
				       vpointer page);
extern void kernel_memory_unmap (struct kernel_page_directory *directory,
				 vpointer from);
extern struct iapi_kernel_memory *kernel_memory_lookup_pagedir (hwpointer ptr);
extern void kernel_memory_free (struct iapi_kernel_memory_pagedir *pd);

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

static void 
memory_map(struct iapi_kernel_memory_pagedir *self,
	   hwpointer page, vpointer to,
	   enum iapi_kernel_memory_access_flags perm)
{
  ATOMIC_INC (get_page_block_ref (page));
  
  kernel_memory_map (self->directory, page, to, perm);
}

static void
memory_remap (struct iapi_kernel_memory_pagedir *self,
	      vpointer page, size_t size,
	      enum iapi_kernel_memory_access_flags perm)
{
  kernel_memory_remap (self->directory, page, size, perm);
}

static void
memory_unmap (struct iapi_kernel_memory_pagedir *self,
	      vpointer page, size_t size)
{
  size_t iter;
  struct iapi_kernel_memory_vm_space_node *vm_space;
  struct iapi_kernel_memory_free_cache_node *free_cache;
  
  iter = page;
  while (iter < page + size)
    {
      int ref;
      hwpointer addr;
      
      kernel_memory_unmap (self->directory, iter);
      
      addr = kernel_memory_lookup (self->directory, page);
      ATOMIC_XADD (get_page_block_ref (addr), -1, ref);
      
      if (!ref)
	{
	  memory_release (kernel_memory_lookup_pagedir (addr),
			  addr);
	}
      
      iter += 0x1000;
    }

  /* lock */
  RB_TREE_LOOKUP (self->vm_space, &vm_space, memory_vm_space_compare, page);
  if (vm_space)
    {
      struct iapi_kernel_memory_free_cache_node *prev;
      struct iapi_kernel_memory_free_cache_node *next;
      
      free_cache =
	(struct iapi_kernel_memory_free_cache_node *)
	((int)vm_space -
	 offsetof (struct iapi_kernel_memory_free_cache_node, node));
      
      if (free_cache->node.region.addr < page)
	{
	  prev = free_cache;
	  next =
	    (struct iapi_kernel_memory_free_cache_node *)
	    ((int)free_cache->node.next -
	     offsetof (struct iapi_kernel_memory_free_cache_node, node));
	}
      else
	{
	  prev =
	    (struct iapi_kernel_memory_free_cache_node *)
	    ((int)free_cache->node.prev -
	     offsetof (struct iapi_kernel_memory_free_cache_node, node));
	  next = free_cache;
	}

      if (prev->node.region.addr + prev->node.region.length == page)
	{
	  if (next->node.region.addr == page + size)
	    {
	      RB_TREE_REMOVE (self->cache,
			      memory_free_cache_rcompare_nodes, next);
	      RB_TREE_REMOVE (self->vm_space,
			      memory_vm_space_compare_nodes, &next->node);
	      free (next);
	      RB_TREE_REMOVE (self->cache,
			      memory_free_cache_rcompare_nodes, prev);
	      prev->node.region.length += size + next->node.region.length;
	      RB_TREE_INSERT (self->cache, prev,
			      memory_free_cache_compare_nodes);
	    }
	  else
	    {
	      RB_TREE_REMOVE (self->cache,
			      memory_free_cache_rcompare_nodes, prev);
	      prev->node.region.length += size;
	      RB_TREE_INSERT (self->cache, prev,
			      memory_free_cache_compare_nodes);
	    }
	}
      else
	{
	  if (next->node.region.addr == page + size)
	    {
	      RB_TREE_REMOVE (self->cache,
			      memory_free_cache_rcompare_nodes, next);
	      RB_TREE_REMOVE (self->vm_space,
			      memory_free_cache_rcompare_nodes, &next->node);
	      next->node.region.addr = page;
	      next->node.region.length += size;
	      RB_TREE_INSERT (self->vm_space, &next->node,
			      memory_free_cache_compare_nodes);
	      RB_TREE_INSERT (self->cache, next,
			      memory_free_cache_compare_nodes);
	    }
	  else
	    {
	      struct iapi_kernel_memory_free_cache_node *node;

	      node = malloc (sizeof (struct iapi_kernel_memory_free_cache_node *));
	      node->node.region.addr = page;
	      next->node.region.length = size;
	      RB_TREE_INSERT (self->vm_space, &node->node,
			      memory_free_cache_compare_nodes);
	      RB_TREE_INSERT (self->cache, node,
			      memory_free_cache_compare_nodes);
	    }
	}
    }
  else
    {
      free_cache = malloc (sizeof (struct iapi_kernel_memory_free_cache_node));
      free_cache->node.region.addr = page;
      free_cache->node.region.length = size;
      RB_TREE_INSERT (self->vm_space, &free_cache->node,
		      memory_vm_space_compare_nodes);
      RB_TREE_INSERT (self->cache, free_cache,
		      memory_free_cache_compare_nodes);
    }
  /* unlock */
}

hwpointer
memory_lookup (struct iapi_kernel_memory_pagedir *self, vpointer page)
{
  kernel_memory_lookup (self->directory, page);
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

static void
iapi_kernel_memory_pagedir_free (struct iapi_kernel_memory_pagedir *pagedir)
{
  /* TODO: Free */
}

void
iapi_kernel_memory_pagedir_init (struct iapi_kernel_memory_pagedir *pagedir)
{
  struct iapi_kernel_memory_free_cache_node *node;
  
  iapi_new ((struct iapi *)pagedir,
	    (iapi_free) iapi_kernel_memory_pagedir_free);
  pagedir->get_page_size = memory_get_page_size;
  pagedir->reserve = memory_reserve;
  pagedir->map = memory_map;
  pagedir->remap = memory_remap;
  pagedir->unmap = memory_unmap;
  pagedir->lookup = memory_lookup;
  node = malloc(sizeof (*node));
  node->node.region.addr = 0;
  node->node.region.length = (size_t)(-1);
  pagedir->vm_space.head = &node->node;
  pagedir->cache.head = node;
  kernel_memory_init (pagedir);
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
		   addr + iter * IAPI_KERNEL_MEMORY_PAGE_SIZE,
		   IAPI_KERNEL_MEMORY_ACCESS_SYSTEM);
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

