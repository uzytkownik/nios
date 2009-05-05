#pragma once

#include <kernel/memory_arch.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      struct hardware_address
      {
      public:
	hardware_address() {}
	hardware_address(hwpointer _ptr) : ptr(_ptr) {}
	hwpointer ptr;
      };
    }
  }
}
