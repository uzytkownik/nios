#ifndef ARCH_KERNEL_MEMORY_HH
#define ARCH_KERNEL_MEMORY_HH

#include "multiboot.h"

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      bool init(multiboot_info *info);
      struct hardware_address
      {
      public:
	typedef int hwpointer;

	hardware_address() : ptr(0) {}
	hardware_address(hwpointer _ptr) : ptr(_ptr) {}
	
	void acquire();
	void release();
	hwpointer ptr;
      };
    };
  };
};
#endif
