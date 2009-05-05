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
