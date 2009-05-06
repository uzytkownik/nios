/*
 * This file is part of nios
 *
 * Copyright (C) 2009 - Maciej Piechotka
 *
 * nios is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include <string.h> //size_t
#include <kiapi/kernel/memory/hardware_address.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      class virtual_memory
      {
	struct vm;
	vm *priv;
      public:
	virtual_memory();
	virtual_memory(const virtual_memory &);
	virtual_memory(vm *);
	size_t page_size() const;
	void *reserve(size_t);
	void *reserve(void *, size_t);
	void map(void *, hardware_address, size_t);
	void unmap(void *, size_t);
	hardware_address lookup(void *) const;
	void use();
	static virtual_memory &get_kernel();
      };
    }
  }
}
