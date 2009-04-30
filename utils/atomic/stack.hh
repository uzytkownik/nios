#pragma once

#include <kiapi/memory/page_allocator.hh>
#include <kiapi/memory/basic_stack.hh>

namespace utils
{
  namespace atomic
  {
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
  }
}
