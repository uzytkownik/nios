#pragma once

#include <kiapi/kernel/memory/memory.hh>
#include <kiapi/kernel/memory/page_allocator.hh>

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      template<typename T>
      class real_page_allocator : public real_page_allocator<T, memory> {};
    }
  }
}
