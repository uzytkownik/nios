/*
 * This file is part of nios
 *
 * Copyright (C) 2009 - Maciej Piechotka
 *
 * nios is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * nios is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nios; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#pragma once

#include <kiapi/kernel/memory/_page_allocator.hh>
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
