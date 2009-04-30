#pragma once

#include <kernel/memory-types.hh>

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
    }
  }
}

#endif
