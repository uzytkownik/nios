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

#include <algorithm>
#include <utils/atomic/base_stack.hh>
#include <kiapi/kernel/memory/hardware_address.hh>
#include <kiapi/kernel/memory/virtual_memory.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      template<size_t size, typename memory = ::kiapi::kernel::memory>
      class real_page_allocator
      {
	struct dummy {};
	static utils::atomic::base_stack<dummy> free_pages;
	typedef typename utils::atomic::base_stack<dummy>::bucket bucket;
	static std::atomic_flag feed_lock;
      public:
	static void *allocate()
	{
	  do
	    {
	      bucket *b = free_pages.pop();
	      if(__builtin_expect(b != NULL, true))
		return b;
	      std::__throw_bad_alloc();
	    }
	  while(true);
	}
	static void deallocate(void *__p)
	{
	  free_pages.push(reinterpret_cast<bucket *>(__p));
	}
      };

      template<typename T, typename memory = kiapi::kernel::memory>
      class page_allocator
      {
	typedef real_page_allocator<sizeof(T), memory> rpage_allocator;
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
	  typedef page_allocator<T1, memory> other;
	};

	_page_allocator() throw() { }
	_page_allocator(const page_allocator &) throw() { }
	template<typename T1>
	_page_allocator(const page_allocator<T1, memory>) throw() { }

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
