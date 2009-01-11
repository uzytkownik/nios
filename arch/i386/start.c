#include <iapi/kernel/memory.h>

void
kstart (unsigned long magic, struct multiboot_info *info)
{
  iapi_kernel_memory_init (info);
  
  while(1)
    __asm__("hlt");
}
