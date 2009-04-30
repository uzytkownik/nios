#pragma once

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      struct hardware_address
      {
      public:
	hardware_address() = default;
	hardware_address(hwpointer _ptr) : ptr(_ptr) {}
	hwpointer ptr;
      };
    }
  }
}
