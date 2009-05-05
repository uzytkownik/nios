#pragma once

#include<string.h> //size_t

namespace kiapi
{
  namespace kernel
  {
    namespace memory
    {
      typedef int hwpointer;
      const size_t memory_page_size = 4096;
      const size_t dma_memory_page_size = 128*1024;
      const size_t dma_memory_start = DMA_START;
      const size_t dma_memory_end = DMA_END;
    }
  }
}
