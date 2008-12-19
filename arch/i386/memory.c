#include <iapi/kernel/memory.h>

struct iapi_kernel_memory i386_memory;

void
iapi_kernel_memory_init (int size, void **reserved)
{
  iapi_new (&i386_memory.iapi, 0);
}

struct iapi_kernel_memory *
iapi_kernel_memory_get_instance ()
{
  return &i386_memory;
}

