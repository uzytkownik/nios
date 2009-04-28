#ifndef KIAPI_KERNEL_MEMORY_HH
#define KIAPI_KERNEL_MEMORY_HH

#include <sys/types.h>

#include "kernel/memory.hh"

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      class memory
      {
	memory(const memory &);
      protected:
	memory() {}
      public:
	virtual size_t page_size() const = 0;
	virtual hardware_address allocate() = 0;
	virtual hardware_address acquire(hardware_address) = 0;
	virtual void release(hardware_address) = 0;
	static memory &get_main();
	static memory &get_dma();
      };
      
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
    };
  };
};

#endif
