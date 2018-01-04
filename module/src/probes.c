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

#include "probes.h"

#include "arm64_defs.h"
#include "cachegrab.h"
#include "probe_types.h"
#include "scope.h"

/*
 * Throughout this file several functions are declared as inline. This
 * is to reduce the overall number of jumps and cache lines this function
 * spans.
 */

int probes_init()
{
	return 0;
}

void probes_term()
{
	return;
}

inline void probes_collect_generic(struct probe *p)
{
	u64 val;
	u64 ctr = ARMV8_IDX_TO_COUNTER(p->pmu_idx);
	asm volatile ("msr pmselr_el0, %0"::"r" (ctr));
	isb();
	asm volatile ("mrs %0, pmxevcntr_el0":"=r" (val));
	p->raw_buf[0] = val;
	p->measure(&p->raw_buf[1], p);
}

inline void probes_process_generic(struct probe *p, struct scope_sample *s)
{
	u64 val, prev;
	unsigned int i, j, nsets, nways;
	size_t offs = s->data_offs;

	nsets = set_count(p->cfg.set_start,
			  p->cfg.set_end, p->cache_shape.num_sets);

	nways = p->cache_shape.associativity;
	for (i = 0; i < nsets; i++) {
		u8 cnt = 0;
		for (j = 0; j < nways; j++) {
			val = p->raw_buf[j * nsets + i + 1];
			prev = p->raw_buf[j * nsets + i];
			if (val > prev)
				cnt += 1;
		}
		s->data[offs + i] = cnt;
	}
	s->data_offs += nsets;
}

inline void probes_refill_generic(struct probe *p)
{
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
	p->refill(p->raw_buf, p);
}

void probes_collect(void *p)
{
	unsigned long interrupt_flags;
	struct scope_sample *s = (struct scope_sample *)p;
	struct probe_l1d *p_l1d;
	struct probe_l1i *p_l1i;
	struct probe_btb *p_btb;
	u8 l1d, l1i, btb;

	local_irq_save(interrupt_flags);

	if (s->scope->activated) {
		p_l1d = &s->scope->l1d_probe;
		p_l1i = &s->scope->l1i_probe;
		p_btb = &s->scope->btb_probe;
		l1d = p_l1d->base.activated;
		l1i = p_l1i->base.activated;
		btb = p_btb->base.activated;

		if (l1d)
			probes_collect_generic(&p_l1d->base);
		if (l1i)
			probes_collect_generic(&p_l1i->base);
		if (btb)
			probes_collect_generic(&p_btb->base);

		s->data_offs = 0;
		if (l1d)
			probes_process_generic(&p_l1d->base, s);
		if (l1i)
			probes_process_generic(&p_l1i->base, s);
		if (btb)
			probes_process_generic(&p_btb->base, s);

		if (btb)
			probes_refill_generic(&p_btb->base);
		if (l1i)
			probes_refill_generic(&p_l1i->base);
		if (l1d)
			probes_refill_generic(&p_l1d->base);

		s->collected = true;
	} else {
		s->collected = false;
	}

	local_irq_restore(interrupt_flags);
	return;
}
