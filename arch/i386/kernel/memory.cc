#include <new>
#include <set>
#include <map>
#include <cstdatomic>
#include <inttypes.h>

using namespace kiapi::kernel::memory;

template<size_t size>  
base_atomic_stack<dummy> real_page_allocator<size>::free_pages;
template<size_t size>
std::atomic_flag real_page_allocator<size>::feed_lock;

template<typename T>
class atomic_stack
{
  base_atomic_stack<T> stack;
  typedef typename base_atomic_stack<T>::bucket bucket;
  page_allocator<bucket> bucket_allocator;
public:
  bool pop(T &__p)
  {
    bucket *b = stack.pop();
    if(__builtin_expect(b == NULL, false))
      {
	return false;
      }
    else
      {
	__p = b->value;
	bucket_allocator.deallocate(b, 1);
	return true;
      }
  }
  void push(T &__p)
  {
    bucket *b = bucket_allocator.allocate(1);
    b->value = __p;
  }
};

template<typename T>
class atomic_stack<T *>
{
  base_atomic_stack<T *> stack;
  typedef typename base_atomic_stack<T *>::bucket bucket;
  page_allocator<bucket> bucket_allocator;
public:
  bool pop(T *&__p)
  {
    bucket *b = stack.pop();
    if (__builtin_expect(b == NULL, false))
      {
	__p = NULL;
	return false;
      }
    else
      {
	__p = b->value;
	bucket_allocator.deallocate(b);
	return true;
      }
  }
  T *pop()
  {
    bucket *b = stack.pop();
    T *t = b->value;
    bucket_allocator.deallocate(b);
    return t;
  }
  void push(T *__p)
  {
    bucket *b = bucket_allocator.allocate(1);
    b->value = __p;
  }
};

class main_memory : memory
{
  atomic_stack<hardware_address> stack;
public:
  size_t page_size()
  {
    return 4096;
  }
  hardware_address allocate()
  {
    hardware_address address;
    stack.pop(address);
    return address;
  }
  hardware_address acquire(hardware_address address)
  {
    // Not implemented yet
    return hardware_address();
  }
  void release(hardware_address address)
  {
    stack.push(address);
  }
};

memory &
memory::get_main()
{
  static char main[sizeof(main_memory)];
  return reinterpret_cast<kiapi::kernel::memory::memory &>(main);
}

class dma_memory : memory
{
  static const size_t _page_size = 128*1024;
  // On most system it should be 96. Linear search should be sufficient
  static const size_t pages = (DMA_END - DMA_START)/_page_size;
  std::atomic<unsigned char> allocated[pages];
  std::atomic<unsigned char> &page (hardware_address address)
  {
    return allocated[(address.ptr - DMA_START)/_page_size];
  }
public:
  size_t page_size()
  {
    return _page_size;
  }
  hardware_address allocate()
  {
    for (size_t i = 0; i < pages; i++)
      {
	unsigned char free = 0, occupied = 1;
	if (allocated[i].compare_exchange_strong(free, occupied))
	  {
	    return (int)(DMA_START+_page_size*i);
	  }
      }
    return hardware_address();
  }
  hardware_address acquire(hardware_address address)
  {
    page(address) += 1;
    return address;
  }
  void release(hardware_address address)
  {
    page(address) -= 1;
  }
};

memory &
memory::get_dma()
{
  static char dma[sizeof(dma_memory)];
  return reinterpret_cast<memory &>(dma);
}

class page_directory
{
  struct page_directory_entry
  {
  private:
    int value;
    static const int address_mask = 0xFFFFFC00;
  public:
    hardware_address address()
    {
      return hardware_address(value & address_mask);
    }
    void address(hardware_address address)
    {
      value = (value & ~address_mask) | (address.ptr & address_mask);
    }
  } entries[1024];
public:

};

// virtual_memory

struct virtual_memory::vm
{
  struct region;
  typedef page_allocator<region> reg_pa;
  typedef std::multimap<size_t, region *, std::less<size_t>, reg_pa> regions_by_size_t;
  typedef std::map<void *, region *, std::greater<void *>, reg_pa> regions_by_start_t;
  struct region
  {
    void *start;
    size_t size;
  };
  regions_by_size_t regions_by_size;
  regions_by_start_t regions_by_start;
  std::atomic_flag vmlock;
  page_allocator<region> region_allocator;
  hardware_address page_directory;
};

virtual_memory::virtual_memory() {}
virtual_memory::virtual_memory(const virtual_memory &) {}
virtual_memory::virtual_memory(virtual_memory::vm *vm)
{
  this->priv = vm;
}

size_t
virtual_memory::page_size() const
{
  return 4096;
}

void *
virtual_memory::reserve(size_t size)
{
  void *page;
  // Spinlock. Should be in global procesor lock
  while(priv->vmlock.test_and_set());
  auto it = priv->regions_by_size.lower_bound(size);
  if (it != priv->regions_by_size.end())
    {
      auto reg = (*it).second;
      priv->regions_by_size.erase(it);
      if (size == reg->size)
	{
	  priv->regions_by_start.erase(reg->start);
	  page = reg->start;
	  priv->region_allocator.deallocate(reg, 1);
	}
      else
	{
	  page = (void *)((char *)reg->start + reg->size - size);
	  reg->size -= size;
	  typedef std::pair<size_t, vm::region *> srpair;
	  priv->regions_by_size.insert(srpair(reg->size, reg));
	}
    }
  else
    {
      page = NULL;
    }
  priv->vmlock.clear();
  return page;
}

void *
virtual_memory::reserve(void *start, size_t size)
{
  void *page;
  // Spinlock. Should be in global procesor lock
  while(priv->vmlock.test_and_set());
  auto it = priv->regions_by_start.lower_bound(start);
  if (it != priv->regions_by_start.end())
    {
      auto reg = (*it).second;

      if ((char *)reg->start + reg->size <= (char *)start + size)
	{
	  auto range = priv->regions_by_size.equal_range(size);
	  for(auto id = range.first; id != range.second; id++)
	    {
	      if((*id).second == reg)
		{
		  priv->regions_by_size.erase(id);
		  break;
		}
	    }
	  if (reg->start == start && reg->size == size)
	    {
	      priv->regions_by_start.erase(it);
	      priv->region_allocator.deallocate(reg, 1);	      
	    }
	  else if (reg->start == start)
	    {
	      priv->regions_by_start.erase(it);
	      reg->start = (void *)((char *)reg->start + size);
	      reg->size -= size;
	      typedef std::pair<size_t, vm::region *> srpair;
	      priv->regions_by_size.insert(srpair(reg->size, reg));
	    }
	  else if ((char *)reg->start + reg->size == (char *)start + size)
	    {
	      reg->size -= size;
	      typedef std::pair<size_t, vm::region *> srpair;
	      priv->regions_by_size.insert(srpair(reg->size, reg));
	    }
	  else
	    {
	      typedef std::pair<size_t, vm::region *> sirpair;
	      typedef std::pair<void *, vm::region *> strpair;
	      auto nreg = priv->region_allocator.allocate(1);
	      nreg->start = (void *)((char *)start + size);
	      nreg->size = (size_t)reg->start+reg->size - (size_t)nreg->start;
	      reg->size = (size_t)start - (size_t)reg->size;
	      priv->regions_by_size.insert(sirpair(nreg->size, nreg));
	      priv->regions_by_size.insert(sirpair(reg->size, reg));
	      priv->regions_by_start.insert(strpair(nreg->start, nreg));
	    }
	}
      else
	{
	  page = NULL;
	}
    }
  else
    {
      page = NULL;
    }
  priv->vmlock.clear();
  return page;
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
  return hardware_address();
}

void
virtual_memory::use()
{
  asm("movl %0, %%cr3"
      : 
      : "r" (this->priv->page_directory)
      : "memory" );
}

virtual_memory &
virtual_memory::get_kernel()
{
  static char kernel[sizeof(virtual_memory)];
  return reinterpret_cast<virtual_memory &>(kernel);
}

bool
kiapi::kernel::memory::init(multiboot_info *info)
{
  atomic_stack<multiboot_info *> a;
  a.push(info);
  return true;
}
