#pragma once

#include <utils/atomic/base_stack.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      template<size_t size>
      class real_page_allocator
      {
	struct dummy {};
	static base_atomic_stack<dummy> free_pages;
	typedef typename base_atomic_stack<dummy>::bucket bucket;
	static std::atomic_flag feed_lock;
	static void feed(void *ptr, size_t length)
	{
	  char *buckets = reinterpret_cast<char *>(ptr);
	  size_t s = std::min(size, sizeof(bucket));
	  bucket *next = NULL;
	  for(size_t i = 0; i < length - s + 1; i += s)
	    {
	      bucket *b = reinterpret_cast<bucket *>(&buckets[i]);
	      b->next = next;
	      next = b;
	    }
	  free_pages.feed(next, reinterpret_cast<bucket *>(buckets));
	}
	static void feed()
	{
	  memory &main = memory::get_main();
	  hardware_address address = main.allocate();
	  size_t page_size = main.page_size();
	  virtual_memory &kvmem = virtual_memory::get_kernel();
	  void *ptr = kvmem.reserve(page_size);
	  kvmem.map(ptr, address, page_size);
	  feed(ptr, page_size);
	}
      public:
	static void *allocate()
	{
	  do
	    {
	      bucket *b = free_pages.pop();
	      if(__builtin_expect(b != NULL, true))
		return b;
	      if(__builtin_expect(!feed_lock.test_and_set(), true))
		{
		  feed();
		  feed_lock.clear();
		}
	    }
	  while(true);
	}
	static void deallocate(void *__p)
	{
	  free_pages.push(reinterpret_cast<bucket *>(__p));
	}
      };

      template<typename T>
      class page_allocator
      {
	typedef real_page_allocator<sizeof(T)> rpage_allocator;
      public:
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef T         value_type;

	template<typename T1>
	struct rebind
	{
	  typedef page_allocator<T1> other;
	};

	page_allocator() throw() { }
	page_allocator(const page_allocator &) throw() { }
	template<typename T1>
	page_allocator(const page_allocator<T1>) throw() { }

	pointer address(reference __x) const { return &__x; }
	const_pointer address(const_reference __x) const { return &__x; }

	pointer allocate(size_type __n, const void * = 0)
	{
	  if (__builtin_expect(__n > 1, false))
	    {
	      std::__throw_bad_alloc();
	    }
	  if (__builtin_expect(__n == 1, true))
	    {
	      return reinterpret_cast<pointer>(rpage_allocator::allocate());
	    }
	  return NULL;
	}

	void deallocate(pointer __p, size_type)
	{
	  rpage_allocator::deallocate(__p);
	}

	size_type max_size() const throw()
	{
	  return 1;
	}

	void construct(pointer __p, const T &__val)
	{
	  ::new((void *)__p) T(__val);
	}

	void destroy(pointer __p)
	{
	  __p->~T();
	}

	inline bool operator==(const page_allocator &)
	{
	  return true;
	}

	inline bool operator!=(const page_allocator &)
	{
	  return false;
	}
      };
    }
  }
}
