#include "kiapi/kernel/memory.hh"
#include <new>
#include <set>

typedef unsigned int uint_t;

using namespace kiapi::kernel::memory;

template<typename T>
class atomic_stack
{
  struct node
  {
    node *next;
    T data;
  };
  volatile node *stack;
  volatile node *free_nodes;
  friend bool kiapi::kernel::memory::init(multiboot_info *);
public:
  atomic_stack() : stack(NULL), free_nodes(NULL) {}
  
  bool pop(node *&no)
  {
    while(node *n = const_cast<node *>(stack))
      {
	if(__sync_bool_compare_and_swap(&stack, n, n->next))
	  {
	    no = n;
	    return true;
	  }
      }
    return false;
  }
  
  bool pop(T &data)
  {
    node *n;

    if(pop(n))
      {
	data = n->data;
	do
	  {
	    n->next = const_cast<node *>(free_nodes);
	  }
	while(__sync_bool_compare_and_swap(&free_nodes, n->next, n));
	return true;
      }
    return false;
  }
  
  void push(node *&n)
  {
    do
      {
	n->next = const_cast<node *>(stack);
      }
    while(__sync_bool_compare_and_swap(&stack, n->next, n));
  }
  
  bool push(T &data)
  {
    while (node *n = const_cast<node *>(free_nodes))
      {
	if (__sync_bool_compare_and_swap(&stack, n, n->next))
	  {
	    n->data = data;
	    push(n);
	    return true;
	  }
      };
    return false;
  }
  
  bool operator>>(node *&data)
  {
    return pop(data);
  }
  
  bool operator>>(T &data)
  {
    return pop(data);
  }
  
  void operator<<(node *&data)
  {
    return push(data);
  }
  
  bool operator<<(T &data)
  {
    return push(data);
  } 
};

static atomic_stack<hardware_address> *_page_stack;
inline atomic_stack<hardware_address> &
get_page_stack()
{
  return *_page_stack;
}

struct page_block
{
  volatile int ref[1024];
};

static page_block **_page_blocks;
static volatile int *
get_ref_count_for(hardware_address addr)
{
  unsigned int ptr = addr.ptr;
  int block = ptr / 0x400000;
  int offset = ptr % 0x400000;
  return &_page_blocks[block]->ref[offset];
}

class main_memory : memory
{
public:
  size_t
  page_size() const
  {
    return 0x1000;
  };

  hardware_address
  allocate()
  {
    atomic_stack<hardware_address> &stack = get_page_stack();
    hardware_address hw;
    stack >> hw;
    volatile int *ref = get_ref_count_for(hw);
    *ref = 0;
    return hw;
  }

  hardware_address
  acquire(hardware_address addr)
  {
    volatile int *ref = get_ref_count_for(addr);
    __sync_add_and_fetch (ref, 1);
    return addr;
  }
  
  void
  release(hardware_address addr)
  {
    volatile int *ref = get_ref_count_for(addr);
    if (!__sync_sub_and_fetch (ref, 1))
      {
	atomic_stack<hardware_address> &stack = get_page_stack();
	stack << addr;
      }
  }
};

static memory *_main;
memory &
memory::get_main()
{
  return *_main;
};

class drm_memory
{

};

static char *_drm;
memory &
memory::get_drm()
{
  return reinterpret_cast<memory &>(_drm);
};

struct free_space_block
{
  void *memory_map;
  size_t length;
};

template<typename T, unsigned int min_fill = 32>
class real_allocator
{
  union node
  {
    node *next;
    T node;
  };
  volatile node *head;
  volatile int size;
  volatile bool feed;
  void
  allocate_more()
  {
    if(__sync_bool_compare_and_swap(&feed, false, true))
      {
	node *nnodes;
	nnodes = (node *)(virtual_memory::get_kernel().reserve(1));
	if(nnodes != NULL)
	  {
	    hardware_address naddr = memory::get_main().allocate();
	    virtual_memory::get_kernel().map(nnodes, naddr, 1);
	    const size_t nodes_in_page = 0x1000/sizeof(node);
	    feed(nnodes, nodes_in_page);
	  }
	feed = false:
	__sync_synchronize();
      }
  }
    void
  feed(node *nnodes, size_t length)
  {
    for(size_t i = 0; i < length; i++)
      {
	nnodes[i].next = &nnodes[i+1];
      }
    node *ohead;
    do
      {
	ohead = const_cast<node *>(head);
	nnodes[nodes_in_page - 1].next = ohead;
      }
    while (__sync_bool_compare_and_swap(&head, ohead, nnodes));
    __sync_add_and_fetch(&size, length);
  }
  friend kiapi::kernel::memory::init(multiboot_info *info);
public:
  real_allocator() : head(0), size(0), feed(false) {}
  T *
  allocate()
  {
    node *next;
    do
      {
	next = const_cast<node *>(head);
	if(next == NULL)
	  allocate_more();
      }
    while (__sync_bool_compare_and_swap(&head, next, next->next));
    if(__sync_sub_and_fetch(&size, 1) < min_fill)
      {
	allocate_more();
      }
    return (T *)next;
  }
  void
  deallocate(T *ptr)
  {
    node *n = (node *)ptr;
    node *ohead;
    do
      {
	ohead = const_cast<node *>(head);
	n->next = ohead;
      }
    while(__sync_bool_compare_and_swap(&head, ohead, n));
  }
};

real_allocator<std::_Rb_tree_node<free_space_block> > *_rallocator;
static inline real_allocator<std::_Rb_tree_node<free_space_block> > &
get_real_allocator()
{
  return *_rallocator;
}

template<typename T>
class vm_allocator
{
public:
  typedef const T *const_pointer;
  typedef const T &const_reference;
  typedef ptrdiff_t difference_type;
  typedef T *pointer;
  typedef T &reference;
  typedef size_t size_type;
  typedef T value_type;

  vm_allocator() {}

  vm_allocator(const vm_allocator<value_type> &) {}

  template <typename TN>
  vm_allocator(const vm_allocator<TN> &) {}

  template <typename TN>
  struct rebind
  {
    typedef vm_allocator<TN> other;
  };

  void
  construct(pointer p, const_reference val)
  {
    new (p) value_type(val);
  }

  void
  destroy(pointer p)
  {
    p->~value_type();
  }
};

template<>
class vm_allocator<std::_Rb_tree_node<free_space_block> >
{
public:
  typedef const std::_Rb_tree_node<free_space_block> *const_pointer;
  typedef const std::_Rb_tree_node<free_space_block> &const_reference;
  typedef ptrdiff_t difference_type;
  typedef std::_Rb_tree_node<free_space_block> *pointer;
  typedef std::_Rb_tree_node<free_space_block> &reference;
  typedef size_t size_type;
  typedef std::_Rb_tree_node<free_space_block> value_type;

  vm_allocator() {}

  vm_allocator(const vm_allocator<value_type> &) {}

  template <typename TN>
  vm_allocator(const vm_allocator<TN> &) {}

  pointer
  address(reference x) const
  {
    return &x;
  }

  const_pointer
  address(const_reference x) const
  {
    return &x;
  }

  pointer
  allocate(size_type n, const void * = 0)
  {
    if(n == 1)
      return get_real_allocator().allocate();
    else
      return NULL;
  }

  void
  deallocate(pointer p, size_type n)
  {
    get_real_allocator().deallocate(p);
  }

  size_type
  max_size() const throw()
  {
    return 1;
  }

  void
  construct(pointer p, const_reference val)
  {
    new (p) value_type(val);
  }

  void
  destroy(pointer p)
  {
    p->~value_type();
  }

  template <typename TN>
  struct rebind
  {
    typedef vm_allocator<TN> other;
  };
};

struct virtual_memory::vm
{
  class compare_free_regions
  {
  public:
    bool operator()(const free_space_block &a, const free_space_block &b)
    {
      return a.memory_map < b.memory_map;
    }
  };

  class compare_cache
  {
  public:
    bool operator()(const free_space_block &a, const free_space_block &b)
    {
      if(a.memory_map == NULL || b.memory_map == NULL)
	return a.length < b.length;
      else
	return a.length < b.length ||
	  (a.length == b.length && a.memory_map < b.memory_map);
    }
  };
  typedef std::set<free_space_block, compare_free_regions, vm_allocator<free_space_block> > free_regions_type;
  free_regions_type free_regions;
  typedef std::set<free_space_block, compare_cache, vm_allocator<free_space_block> > cache_type;
  cache_type cache;
};

size_t
virtual_memory::page_size() const
{
  return 0x1000;
}

void *
virtual_memory::reserve(size_t size)
{
  free_space_block block = {NULL, size};
  vm::cache_type::iterator iter = priv->cache.lower_bound(block);
  if(iter != priv->cache.end())
    {
      void *addr = iter->memory_map;
      if(iter->length > size)
	{
	  priv->free_regions.erase(*iter);
	  priv->cache.erase(iter);
	}
      else
	{
	  free_space_block tmp = *iter;
	  tmp.memory_map = (void *)((int)tmp.memory_map + size * 0x1000);
	  tmp.length -= size;
	  priv->cache.erase(iter);
	  priv->cache.insert(tmp);
	}
      return addr;
    }
  return NULL;
}

void *
virtual_memory::reserve(void *place, size_t size)
{
  free_space_block block = {place, NULL};
  vm::free_regions_type::iterator iter = priv->free_regions.lower_bound(block);
  if(iter != priv->free_regions.end())
    {
      free_space_block &fsb = const_cast<free_space_block &>(*iter);
      if(fsb.memory_map == place)
	{
	  if (fsb.length > size)
	    {
	      priv->cache.erase(fsb);
	      if(fsb.length == size)
		{
		  priv->free_regions.erase(iter);
		}
	      else if (fsb.length > size)
		{
		  fsb.memory_map = (void *)((int)fsb.memory_map + 0x1000 * size);
		  fsb.length -= size;
		  priv->cache.insert(fsb);
		}
	      return place;
	    }
	}
      else if ((int)fsb.memory_map + fsb.length * 0x1000 ==
	       (int)place + size * 0x1000)
	{
	  priv->cache.erase(fsb);
	  fsb.length -= size;
	  priv->cache.insert(fsb);
	  return place;
	}
      else if ((int)fsb.memory_map + fsb.length * 0x1000 >
	       (int)place + size * 0x1000)
	{
	  priv->cache.erase(fsb);
	  fsb.length = ((int)place - (int)fsb.memory_map)/0x1000;
	  priv->cache.insert(fsb);
	  free_space_block nblock = {
	    (void *)((int)place + size * 0x1000),
	    (((int)fsb.memory_map + fsb.length * 0x1000) -
	     ((int)place + size * 0x1000))
	  };
	  priv->cache.insert(nblock);
	  priv->free_regions.insert(nblock);
	}
    }
  return NULL;
}

void
virtual_memory::map(void *, hardware_address, size_t)
{

}

void
virtual_memory::unmap(void *, size_t)
{

}

hardware_address
virtual_memory::lookup(void *) const
{

}

static virtual_memory *_virtual_memory;
virtual_memory &
virtual_memory::get_kernel()
{
  return *_virtual_memory;
}

void
hardware_address::acquire()
{
  if (this->ptr > DMA_END)
    {
      memory::get_main().acquire(*this);
    }
  else if (this->ptr > DMA_START)
    {
      memory::get_drm().acquire(*this);
    }
}

void
hardware_address::release()
{
  if (this->ptr > DMA_END)
    {
      memory::get_main().release(*this);
    }
  else if (this->ptr > DMA_START)
    {
      memory::get_drm().release(*this);
    }
}

bool
kiapi::kernel::memory::init(multiboot_info *info)
{
  long eom = 0xC0000000 + DMA_END;
  // Initialize page stack
  _page_stack = reinterpret_cast<atomic_stack<hardware_address> *>(eom);
  new (_page_stack) atomic_stack<hardware_address>();
  eom += sizeof(atomic_stack<hardware_address>);
  if (eom % 4 != 0) // Align 4
    eom += 4 - (eom % 4);
  // Init main memory
  _main = reinterpret_cast<memory *>(eom);
  new (_main) main_memory();
  eom += sizeof(main_memory);
  if (eom % 4 != 0) // Align 4
    eom += 4 - (eom % 4);
  // Initialize 'real allocator'
  _rallocator = eom;
  typedef real_allocator<std::_Rb_tree_node<free_space_block> > ralloc;
  new (_rallocator) ralloc();
  eom += sizeof(*_rallocator);
  _rallocator->feed(reinterpret_cast<ralloc::node>(eom),
		    (0x2000 - eom % 0x1000)/sizeof(ralloc::node));
  eom = eom & ~(0x1000 - 1) + 0x2000;
  // Initialize page blocks
  _page_blocks = reinterpret_cast<page_block **>(eom);
  eom += 0x1000;
  for(int i = 0; i < 1024, i++)
    _page_blocks[i] = NULL;
  _page_blocks[(eom + 0x1000) / 0x400000]
    = reinterpret_cast<page_block *>(eom);
  for(int i = 0; i < 1024, i++)
    _page_blocks[(eom + 0x1000) / 0x400000]->ref[i] = 0;
  eom += 0x1000;
  // Fill the table
  
  return true;
}
