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

extern "C" {

#pragma pack(1)

struct multiboot_modules
{
  void *start;
  void *end;
  char *string;
  int reserved;
};

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
    struct multiboot_modules *address;
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

};

