/**
 * This file is part of the Cachegrab kernel module.
 *
 * Copyright (C) 2017 NCC Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cachegrab.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 Version 1.0
 Keegan Ryan, NCC Group
*/

#ifndef MEMORY_H__
#define MEMORY_H__

#include <linux/kernel.h>

#define MEM_FLAG_UNCACHED   0x01
#define MEM_FLAG_EXECUTABLE 0x02

/**
 * Structure to a section of memory allocated with certain page table flags.
 */
struct cgmem {
	size_t size;
	unsigned int page_count;
	unsigned int page_order;
	struct page *raw_pages;
	void *mapped_addr;
};

/**
 * Initializes an instance of a memory structure.
 *
 * @param m The memory structure to initialize.
 * @param sz The size of the memory to create
 * @param flags Flags describing how the memory should be allocated.
 * @returns A pointer to the usable memory on success, NULL otherwise.
 */
void *cgmem_init(struct cgmem *m, size_t sz, unsigned int flags);

/**
 * Deinitializes a memory structure instance.
 */
void cgmem_deinit(struct cgmem *m);

#endif
