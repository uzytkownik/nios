#pragma once

#include <utils/atomic/_stack.hh>
#include <kiapi/kernel/memory/memory.hh>

namespace utils
{
    namespace atomic
    {
      template<typename T>
      class stack : public _stack<T, kiapi::kernel::memory::memory> {};
    }
}
