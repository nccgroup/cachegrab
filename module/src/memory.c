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

#include "memory.h"

#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "cachegrab.h"

pgprot_t pgprot_from_flags(unsigned int flags)
{
	pgprot_t base;
	if (flags & MEM_FLAG_EXECUTABLE) {
		base = PAGE_KERNEL_EXEC;
	} else {
		base = PAGE_KERNEL;
	}
	if (flags & MEM_FLAG_UNCACHED) {
		base = __pgprot_modify(base,
				       PTE_ATTRINDX_MASK,
				       PTE_ATTRINDX(MT_DEVICE_nGnRnE));
	}
	return base;
}

void *cgmem_init(struct cgmem *m, size_t sz, unsigned int flags)
{
	struct page **pagelist = NULL;
	pgprot_t prot;
	size_t tmp_sz;
	uint64_t tmp_vaddr;
	unsigned int i;

	if (m == NULL || sz == 0)
		return NULL;

	DEBUG("Here to initialize %zu bytes of (%02x) memory", sz, flags);
	memset(m, 0, sizeof(struct cgmem));

	m->size = sz;
	m->page_count = DIV_ROUND_UP(m->size, PAGE_SIZE);
	m->page_order = order_base_2(m->page_count);
	m->raw_pages = alloc_pages(GFP_KERNEL, m->page_order);
	if (m->raw_pages == NULL) {
		INFO("Unable to allocate enough pages.");
		goto err;
	}

	tmp_sz = m->page_count * sizeof(struct page *);
	pagelist = (struct page **)kmalloc(tmp_sz, GFP_KERNEL);
	if (pagelist == NULL) {
		INFO("Unable to allocate pagelist structure.");
		goto err;
	}

	prot = pgprot_from_flags(flags);
	tmp_vaddr = (uint64_t) page_address(m->raw_pages);
	for (i = 0; i < m->page_count; i++) {
		pagelist[i] = virt_to_page(tmp_vaddr + i * PAGE_SIZE);
	}
	m->mapped_addr = vmap(pagelist, m->page_count, VM_MAP, prot);
	if (m->mapped_addr == NULL) {
		INFO("Memory mapping failed.");
		goto err;
	}

	kfree(pagelist);
	DEBUG("Mapped address is %p", m->mapped_addr);
	return m->mapped_addr;
 err:
	if (pagelist)
		kfree(pagelist);
	cgmem_deinit(m);
	return NULL;
}

void cgmem_deinit(struct cgmem *m)
{
	if (m->mapped_addr) {
		vunmap(m->mapped_addr);
		m->mapped_addr = NULL;
	}
	if (m->raw_pages) {
		__free_pages(m->raw_pages, m->page_order);
		m->raw_pages = NULL;
	}
	return;
}
