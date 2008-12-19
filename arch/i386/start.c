#include <iapi/kernel/memory.h>

void
kstart (unsigned long magic, unsigned long addr)
{
  void *reserved[64];

  reserved[0] = 0; //TODO: Handle it from multiboot
  
  iapi_kernel_memory_init (0x02000000, reserved);
  
  while(1)
    __asm__("hlt");
}
