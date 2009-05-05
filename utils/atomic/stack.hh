#pragma once

#include <kiapi/kernel/memory/page_allocator.hh>
#include <utils/atomic/base_stack.hh>

namespace utils
{
  namespace atomic
  {
    template<typename T, typename memory>
    class _stack
    {
      base_stack<T> stack;
      typedef typename base_stack<T>::bucket bucket;
      kiapi::kernel::memory::_page_allocator<bucket, memory> bucket_allocator;
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
