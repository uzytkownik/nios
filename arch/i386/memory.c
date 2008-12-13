#include "memory.h"

/* simple rename */
#define page_directory (end)

/* It is already aligned */
extern unsigned int **end;

void
i386_mem_init ()
{
  /* Clear the page directory */
  for (int i = 0; i < 1024; i++)
    {
      page_directory[i] = 0;
    }

  /* Sets the first page table */
  page_directory[0] = (unsigned int *)(page_directory + 0x1000);
  for(unsigned int i = 0; i < 1024; i++)
    {
      page_directory[0][i] = i * 0x1000 | 3;
    }
  page_directory[0] = (unsigned int *)((unsigned int)page_directory[0] | 3);
  
  /* Enable pagging */
  __asm__ volatile ("movl %%ebx, %%cr3\n"
		    "\tmovl %%cr0, %%ebx\n"
		    "\tor $0x80000000, %0\n"
		    "\tmovl %%ebx, %%cr0"
		    :
		    : "b"(page_directory)
		    : "0" );
}
