#include <iapi/kernel/memory.h>

#pragma pack(1)
struct multiboot_memory_map
{
  unsigned int size;
  unsigned long long addr;
  unsigned long long length;
  unsigned int type;
};

struct multiboot_drive
{
  unsigned int size;
  unsigned char number;
  unsigned char mode;
  unsigned short cylinders;
  unsigned char heads;
  unsigned char sectors;
  unsigned short ports[];
};

struct multiboot_info
{
  unsigned int flags;
  struct {
    unsigned int lower;
    unsigned int upper;
  } memory;
  struct {
    char drive;
    char part1;
    char part2;
    char part3;
  } boot_device;
  char *commandline;
  struct {
    unsigned int length;
    void *address;
  } modules;
  char padding[16];
  struct {
    unsigned int length;
    struct multiboot_memory_map *addr;
  } mmap;
  void *config_table;
  char *boot_loader_name;
  struct
  {
    unsigned short version;
    unsigned short cseg;
    unsigned int offset;
    unsigned short cseg_16;
    unsigned short dseg;
    unsigned short flags;
    unsigned short cseg_len;
    unsigned short cseg_16_len;
    unsigned short dseg_len;
  } apm;
};

#pragma pack()

void
kstart (unsigned long magic, struct multiboot_info *info)
{
  void *usable[64];
  unsigned int usable_lengths[64];
  unsigned int usable_length;
  struct multiboot_memory_map *mmap;

  usable_length = 0;
  mmap = info->mmap.addr;
  while(usable_length < 64 && (int)mmap < (int)info->mmap.addr + info->mmap.length)
    {
      if (mmap->type == 1)
	{
	  usable[usable_length] = (void *)((unsigned int)mmap->addr);
	  usable_lengths[usable_length] = (unsigned int)mmap->length;
	  usable_length++;
	}
      
      mmap = (struct multiboot_memory_map *)((unsigned int)mmap + mmap->size + sizeof(unsigned int));
    }
  
  //iapi_kernel_memory_init (usable_length, usable, usable_lengths);
  
  while(1)
    __asm__("hlt");
}
