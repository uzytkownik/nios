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

#include <kernel/memory_arch.hh>
#include <utils/atomic/stack.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      class memory
      {
	static utils::atomic::stack<hardware_address, memory> stack;
      public:
	static const int page_size = memory_page_size;
	hardware_address
	allocate()
	{
	  hardware_address address;
	  stack.pop(address);
	  return address;
	}
	void
	release(hardware_address address)
	{
	  stack.push(address);
	}
      };

      class dma_memory
      {
      public:
	const static size_t page_size = dma_memory_page_size;
      private:
	const static size_t pages = (dma_memory_end - dma_memory_start)/page_size;
	static std::atomic<unsigned char> allocated[pages];
	static std::atomic<unsigned char> &
	page (hardware_address address)
	{
	  return allocated[(address.ptr - dma_memory_start)/page_size];
	}
      public:
	hardware_address allocate()
	{
	  for (size_t i = 0; i < pages; i++)
	    {
	      unsigned char free = 0, occupied = 1;
	      if (allocated[i].compare_exchange_strong(free, occupied))
		{
		  return (int)(dma_memory_start+page_size*i);
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
    }
  }
}
