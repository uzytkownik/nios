#pragma once

#include <cstdatomic>

namespace utils
{
  namespace atomic
  {
    template<typename T>
    class basic_stack
    {
    public:
      struct bucket
      {
	bucket *next;
	T value;
      };
    private:
      std::atomic<bucket *> head;
    public:
      bucket *pop()
      {
	bucket *old_head = head.load();
	bucket *next;
	do
	  {
	    if (__builtin_expect(old_head == NULL, false))
	      return NULL;
	    next = old_head->next;
	  }
	while (head.compare_exchange_strong(old_head, next));
	return old_head;
      }
      void push(bucket *b)
      {
	do
	  {
	    b->next = head.load();
	  }
	while(head.compare_exchange_strong(b->next, b));
      }
      void push(bucket *first, bucket *last)
      {
	do
	  {
	    last->next = head.load();
	  }
	while(head.compare_exchange_strong(last->next, first))
      }
    };
  }
}
