/*
 * This file is part of nios
 *
 * Copyright (C) 2009 - Maciej Piechotka
 *
 * nios is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nios is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nios; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

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
