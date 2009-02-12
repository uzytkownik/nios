#include <kiapi/kernel/memory.hh>

extern "C" void kstart (unsigned long magic, struct multiboot_info *info);

void
kstart (unsigned long magic, struct multiboot_info *info)
{
  kiapi::kernel::memory::init(info);
  while(1)
    __asm__("hlt");
}
