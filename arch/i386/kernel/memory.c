#include <iapi/kernel/memory.h>

void
kernel_map (struct kernel_page_directory *directory,
	    hwpointer page, vpointer to)
{

}

hwpointer
kernel_lookup (struct kernel_page_directory *directory,
	       vpointer page)
{

}

void
kernel_unmap (struct kernel_page_directory *directory,
	      hwpointer page, vpointer from)
{

}
