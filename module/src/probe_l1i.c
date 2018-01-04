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

#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>

#include "arm64_defs.h"
#include "cachegrab.h"

void probe_l1i_init(struct probe_l1i *p)
{
	probe_generic_init((struct probe *)p);
}

enum CGState probe_l1i_attach(struct probe_l1i *p, int cpu,
			      struct cache_shape *s)
{
	enum CGState err;
	struct probe_params params;
	size_t mem_needed;
	unsigned int flags;
	u64 offs;

	if (p->base.attached) {
		INFO("Probe already attached.");
		return CG_PROBE_ALREADY_CONNECTED;
	}
	if (s == NULL) {
		INFO("Cache shape should not be null.");
		return CG_BAD_ARG;
	}
	if (s->line_size < 2 * INS_SIZE || s->line_size % INS_SIZE != 0) {
		INFO("L1I Line size must be at least 8 to fit entire gadget.");
		return CG_BAD_ARG;
	}
	// Add 1 to way so that we can move around a smaller buffer at any
	// set offset. E.G. if set_start is nsets - 1, then just shift
	// everything over.
	mem_needed = s->num_sets * (s->associativity + 1) * s->line_size;
	if (s->line_size - 2 * INS_SIZE == 0)
		// Make room for return if needed
		mem_needed += INS_SIZE;
	flags = MEM_FLAG_EXECUTABLE;
	if (cgmem_init(&p->exec_mem, mem_needed, flags) == NULL) {
		err = CG_NO_MEM;
		goto fail_execmem;
	}
	// Compute which set our first measurement comes from
	offs = (u64) p->exec_mem.mapped_addr;
	offs /= p->base.cache_shape.line_size;
	offs %= p->base.cache_shape.num_sets;
	p->base.set_offset = (unsigned int)offs;

	params.target_cpu = cpu;
	params.type = PERF_TYPE_RAW;
	params.conf = EVENT_L1_ICACHE_REFILL;

	params.cache_shape = s;
	err = probe_generic_attach((struct probe *)p, &params);
	if (err != CG_OK)
		goto fail_generic;

	p->base.measure = (probe_func) probe_l1i_measure;
	p->base.refill = (probe_func) probe_l1i_refill;
	return 0;
 fail_generic:
	cgmem_deinit(&p->exec_mem);
 fail_execmem:
	return err;
}

void probe_l1i_detach(struct probe_l1i *p)
{
	probe_generic_detach((struct probe *)p);
	cgmem_deinit(&p->exec_mem);
}

int probe_l1i_configure(struct probe_l1i *p, struct probe_config *cfg)
{
	int ret;
	unsigned int way, nways, set, nsets, line_size;
	unsigned int start, end, ext, numext;
	unsigned int s_count;
	uint32_t *func;
	unsigned int ind, offset;

	ret = probe_generic_configure((struct probe *)p, cfg);
	if (ret < 0)
		return ret;

	nways = p->base.cache_shape.associativity;
	nsets = p->base.cache_shape.num_sets;
	line_size = p->base.cache_shape.line_size;
	start = p->base.cfg.set_start;
	end = p->base.cfg.set_end;
	numext = (line_size / INS_SIZE) - 2;
	s_count = set_count(start, end, nsets);

	offset = (start - p->base.set_offset + nsets) % nsets;
	offset *= line_size;
	func = (uint32_t *) ((u8 *) p->exec_mem.mapped_addr + offset);

	ind = 0;
	for (way = 0; way < nways; way++) {
		for (set = 0; set < nsets; set++) {
			if (set < s_count) {
				func[ind++] = READ_EVCNTR;
				func[ind++] = STORE_VALUE;
			} else {
				func[ind++] = NOP;
				func[ind++] = NOP;
			}
			if (set < nsets - 1 || way < nways - 1) {
				for (ext = 0; ext < numext; ext++) {
					func[ind++] = NOP;
				}
			}
		}
	}
	func[ind] = RET;

	p->base.measure = (probe_func) func;
	p->base.refill = (probe_func) func;

	flush_cache_all();
	return 0;
}

void probe_l1i_measure(u64 * buf, struct probe_l1i *p)
{
}

void probe_l1i_refill(u64 * buf, struct probe_l1i *p)
{
}
