void
kstart (unsigned long magic, unsigned long addr)
{
  i386_mem_init ();
  
  while(1)
    __asm__("hlt");
}
