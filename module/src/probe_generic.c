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

#include <linux/slab.h>

#include "cachegrab.h"

void probe_generic_init(struct probe *p)
{
	memset(p, 0, sizeof(struct probe));
}

int is_probe_attached(struct probe *p)
{
	return (p->attached) ? 1 : 0;
}

enum CGState probe_generic_attach(struct probe *p, struct probe_params *params)
{
	struct perf_event_attr pattr;
	unsigned int num_sets;
	size_t meas_cnt;
	enum CGState err;

	if (p->attached)
		return CG_PROBE_ALREADY_CONNECTED;

	// Handle cache related steps
	memcpy(&p->cache_shape, params->cache_shape, sizeof(p->cache_shape));
	num_sets = p->cache_shape.num_sets;
	meas_cnt = p->cache_shape.associativity * num_sets + 1;
	DEBUG("This probe has count %zu", meas_cnt);
	p->raw_buf = (u64 *) kmalloc(meas_cnt * sizeof(u64), GFP_KERNEL);
	if (p->raw_buf == NULL)
		return CG_NO_MEM;
	// Fill raw buf to page in all pages
	memset(p->raw_buf, 0, meas_cnt * sizeof(u64));

	// Handle PMU related steps
	memset(&pattr, 0, sizeof(struct perf_event_attr));
	pattr.type = params->type;
	pattr.size = sizeof(struct perf_event_attr);
	pattr.config = params->conf;
	pattr.pinned = 1;

	p->pevent =
	    perf_event_create_kernel_counter(&pattr, params->target_cpu, NULL,
					     NULL, NULL);

	if (IS_ERR(p->pevent)) {
		WARNING("Unable to capture performance counter.");
		err = CG_INTERNAL_ERR;
		goto fail;
	}

	perf_event_disable(p->pevent);

	// Finish
	p->attached = true;

	return CG_OK;
 fail:
	if (p->raw_buf)
		kfree(p->raw_buf);
	return err;
}

void probe_generic_detach(struct probe *p)
{
	if (!p->attached)
		return;
	if (p->activated)
		probe_generic_deactivate(p);
	if (p->pevent) {
		perf_event_release_kernel(p->pevent);
		p->pevent = NULL;
		DEBUG("Disabled event.");
	}
	if (p->raw_buf) {
		kfree(p->raw_buf);
	}
	p->attached = false;
}

int probe_generic_configure(struct probe *p, struct probe_config *cfg)
{
	unsigned int start, end, max;
	struct probe_config default_config;

	if (!p->attached) {
		INFO("Probe must be attached before it is configured.");
		return -1;
	}
	if (cfg == NULL) {
		INFO("Setting to default configuration.");
		default_config.set_start = 0;
		default_config.set_end = p->cache_shape.num_sets;
		cfg = &default_config;
	} else {
		start = cfg->set_start;
		end = cfg->set_end;
		max = p->cache_shape.num_sets;
		if (start >= max || end - 1 >= max || start == end) {
			INFO("Invalid probe configuration.");
			return -1;
		}
	}

	memcpy(&p->cfg, cfg, sizeof(struct probe_config));
	return 0;
}

struct cache_shape *probe_generic_get_cache_shape(struct probe *p)
{
	return p->attached ? &p->cache_shape : NULL;
}

struct probe_config *probe_generic_get_config(struct probe *p)
{
	return p->attached ? &p->cfg : NULL;
}

int is_probe_activated(struct probe *p)
{
	return (p->activated) ? 1 : 0;
}

int probe_generic_activate(struct probe *p)
{
	perf_event_enable(p->pevent);
	DEBUG("counter index is %d", p->pevent->hw.idx);
	p->pmu_idx = p->pevent->hw.idx;
	p->activated = true;
	return 0;
}

void probe_generic_deactivate(struct probe *p)
{
	u64 enabled, running, value;

	if (!is_probe_activated(p))
		return;

	value = perf_event_read_value(p->pevent, &enabled, &running);
	DEBUG("val %llu, enabled %llu, running %llu", value, enabled, running);

	perf_event_disable(p->pevent);

	p->activated = false;
}

size_t probe_generic_sample_size(struct probe * p)
{
	if (is_probe_attached(p))
		return set_count(p->cfg.set_start,
				 p->cfg.set_end, p->cache_shape.num_sets);
	else
		return 0;
}
