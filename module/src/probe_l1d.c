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

#include "probe_types.h"

#include <linux/delay.h>
#include <linux/kallsyms.h>

#include "arm64_defs.h"
#include "cachegrab.h"

void probe_l1d_init(struct probe_l1d *p)
{
	probe_generic_init((struct probe *)p);
}

enum CGState probe_l1d_attach(struct probe_l1d *p, int cpu,
			      struct cache_shape *s)
{
	enum CGState err;
	struct probe_params params;
	u64 offs;

	if (p->base.attached)
		return CG_PROBE_ALREADY_CONNECTED;

	// We use _text as a reference since it is likely to be the
	// beginning of a large number of physically contiguous pages
	p->kernel_code_base = (u8 *) kallsyms_lookup_name("_text");
	if (p->kernel_code_base == NULL) {
		WARNING("Unable to locate start of kernel code");
		return CG_INTERNAL_ERR;
	}
	// Compute which set our first measurement comes from
	offs = (u64) virt_to_phys(p->kernel_code_base);
	offs /= p->base.cache_shape.line_size;
	offs %= p->base.cache_shape.num_sets;
	p->base.set_offset = (unsigned int)offs;

	params.target_cpu = cpu;
	params.type = PERF_TYPE_RAW;
	params.conf = EVENT_L1_DCACHE_REFILL;

	params.cache_shape = s;
	err = probe_generic_attach((struct probe *)p, &params);
	if (err != CG_OK)
		return err;

	p->base.measure = (probe_func) probe_l1d_measure;
	p->base.refill = (probe_func) probe_l1d_refill;
	return CG_OK;
}

void probe_l1d_detach(struct probe_l1d *p)
{
	probe_generic_detach((struct probe *)p);
}

int probe_l1d_configure(struct probe_l1d *p, struct probe_config *cfg)
{
	return probe_generic_configure((struct probe *)p, cfg);
}

void probe_l1d_measure(u64 * buf, struct probe_l1d *p)
{
	unsigned int way, nways = p->base.cache_shape.associativity;
	unsigned int set, nsets = p->base.cache_shape.num_sets;
	unsigned int line_size = p->base.cache_shape.line_size;
	unsigned int start = p->base.cfg.set_start;
	unsigned int s_count;
	unsigned int offset_from_base;
	volatile u8 *addr;
	volatile u8 *addr_base;
	u64 val = 0;
	s_count = set_count(start, p->base.cfg.set_end, nsets);

	offset_from_base = (start - p->base.set_offset + nsets) % nsets;
	addr_base = p->kernel_code_base + line_size * offset_from_base;
	for (way = 0; way < nways; way++) {
		addr = addr_base + line_size * way * nsets;
		for (set = 0; set < s_count; set++) {
			*addr;
			dsb(sy);
			isb();
			asm volatile ("mrs %0, pmxevcntr_el0":"=r" (val));
			*buf++ = val;
			addr += line_size;
		}
	}
}

void probe_l1d_refill(u64 * buf, struct probe_l1d *p)
{
	unsigned int way, nways = p->base.cache_shape.associativity;
	unsigned int set, nsets = p->base.cache_shape.num_sets;
	unsigned int line_size = p->base.cache_shape.line_size;
	unsigned int start, s_count;
	volatile u8 *addr, *addr_base;
	unsigned int offset_from_base;

	start = p->base.cfg.set_start;
	s_count = set_count(start, p->base.cfg.set_end, nsets);

	offset_from_base = (start - p->base.set_offset + nsets) % nsets;
	addr_base = p->kernel_code_base + line_size * offset_from_base;
	for (way = 0; way < nways; way++) {
		addr = addr_base + line_size * way * nsets;
		for (set = 0; set < s_count; set++) {
			*addr;
			addr += line_size;
		}
	}
}
