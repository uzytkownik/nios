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

#include <cstdatomic>

namespace utils
{
  namespace atomic
  {
    template<typename T>
    class base_stack
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
	while(head.compare_exchange_strong(last->next, first));
      }
    };
  }
}
