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

#include "scope.h"

#include <linux/delay.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "probes.h"

static struct scope s;
static struct list_head prepared_samples;
static struct list_head collected_samples;
static unsigned int collected_count;

int scope_init()
{
	// Initialize all probes
	probe_generic_init((struct probe *)&s.l1d_probe);
	probe_generic_init((struct probe *)&s.l1i_probe);
	probe_generic_init((struct probe *)&s.btb_probe);

	s.created = false;
	s.activated = false;

	INIT_LIST_HEAD(&prepared_samples);
	INIT_LIST_HEAD(&collected_samples);
	collected_count = 0;

	return 0;
}

void scope_term()
{
	scope_destroy();
	return;
}

void scope_get_config(struct scope_configuration *cfg)
{
	cfg->created = false;

	if (s.created) {
		cfg->created = true;
		cfg->target_cpu = s.target_cpu;
		cfg->l1d_attached = (0 != is_probe_attached(&s.l1d_probe.base));
		cfg->l1i_attached = (0 != is_probe_attached(&s.l1i_probe.base));
		cfg->btb_attached = (0 != is_probe_attached(&s.btb_probe.base));
	} else {
		cfg->created = false;
		cfg->target_cpu = -1;
		cfg->l1d_attached = false;
		cfg->l1i_attached = false;
		cfg->btb_attached = false;
	}
}

enum CGState scope_create(int target_cpu)
{
	if (s.created) {
		INFO("Scope already created.");
		return CG_SCOPE_ALREADY_CONNECTED;
	}

	s.target_cpu = target_cpu;
	s.created = true;

	return CG_OK;
}

void scope_destroy()
{
	if (!s.created)
		return;

	scope_flush();

	scope_deactivate();

	// Terminate any probes still attached to the scope
	if (is_probe_attached((struct probe *)&s.l1d_probe))
		probe_l1d_detach(&s.l1d_probe);

	if (is_probe_attached((struct probe *)&s.l1i_probe))
		probe_l1i_detach(&s.l1i_probe);

	if (is_probe_attached((struct probe *)&s.btb_probe))
		probe_btb_detach(&s.btb_probe);

	s.created = false;
}

enum CGState scope_attach_probe(enum probe_type t, struct cache_shape *sh)
{
	int ret;

	if (!s.created) {
		INFO("Scope not created.");
		return CG_SCOPE_NOT_CONNECTED;
	}

	switch (t) {
	case PROBE_TYPE_L1D:
		ret = probe_l1d_attach(&s.l1d_probe, s.target_cpu, sh);
		break;
	case PROBE_TYPE_L1I:
		ret = probe_l1i_attach(&s.l1i_probe, s.target_cpu, sh);
		break;
	case PROBE_TYPE_BTB:
		ret = probe_btb_attach(&s.btb_probe, s.target_cpu, sh);
		break;
	default:
		DEBUG("Probe type not recognized.");
		ret = CG_BAD_ARG;
	}

	if (ret == CG_OK) {
		INFO("Successfully attached probe to target core.");
		scope_configure_probe(t, NULL);
	} else {
		INFO("Unsuccessfully attached probe to target core.");
	}
	return ret;
}

void scope_detach_probe(enum probe_type type)
{
	if (!s.created) {
		INFO("Scope not created.");
		return;
	}

	switch (type) {
	case PROBE_TYPE_L1D:
		probe_l1d_detach(&s.l1d_probe);
		break;
	case PROBE_TYPE_L1I:
		probe_l1i_detach(&s.l1i_probe);
		break;
	case PROBE_TYPE_BTB:
		probe_btb_detach(&s.btb_probe);
		break;
	default:
		DEBUG("Probe type not recognized.");
		break;
	}
	INFO("Detached");
}

struct probe *scope_get_probe(enum probe_type type)
{
	struct probe *ret = NULL;

	if (!s.created) {
		INFO("Scope not created.");
		return NULL;
	}

	switch (type) {
	case PROBE_TYPE_L1D:
		ret = (struct probe *)&s.l1d_probe;
		break;
	case PROBE_TYPE_L1I:
		ret = (struct probe *)&s.l1i_probe;
		break;
	case PROBE_TYPE_BTB:
		ret = (struct probe *)&s.btb_probe;
		break;
	default:
		DEBUG("Probe type not recognized.");
		return NULL;
	}

	return is_probe_attached(ret) ? ret : NULL;
}

int scope_configure_probe(enum probe_type type, struct probe_config *cfg)
{
	int ret;

	if (!s.created) {
		INFO("Scope not created.");
		return -1;
	}

	switch (type) {
	case PROBE_TYPE_L1D:
		ret = probe_l1d_configure(&s.l1d_probe, cfg);
		break;
	case PROBE_TYPE_L1I:
		ret = probe_l1i_configure(&s.l1i_probe, cfg);
		break;
	case PROBE_TYPE_BTB:
		ret = probe_btb_configure(&s.btb_probe, cfg);
		break;
	default:
		DEBUG("Probe type not recognized.");
		ret = -1;
	}

	if (ret < 0) {
		WARNING("Probe configuration failed.");
	} else {
		DEBUG("Configured probe");
	}
	return ret;
}

int scope_activate()
{
	if (!s.created) {
		INFO("Scope not created.");
		return -1;
	}
	DEBUG("Activating scope");

	if (is_probe_attached((struct probe *)&s.l1d_probe)) {
		probe_generic_activate((struct probe *)&s.l1d_probe);
	}

	if (is_probe_attached((struct probe *)&s.l1i_probe)) {
		probe_generic_activate((struct probe *)&s.l1i_probe);
	}

	if (is_probe_attached((struct probe *)&s.btb_probe)) {
		probe_generic_activate((struct probe *)&s.btb_probe);
	}

	s.activated = true;

	return 0;
}

void scope_deactivate()
{
	if (!s.created) {
		INFO("Scope not created.");
		return;
	}

	DEBUG("Deactivating scope.");

	s.activated = false;

	if (is_probe_attached((struct probe *)&s.l1d_probe)) {
		probe_generic_deactivate((struct probe *)&s.l1d_probe);
	}
	if (is_probe_attached((struct probe *)&s.l1i_probe)) {
		probe_generic_deactivate((struct probe *)&s.l1i_probe);
	}
	if (is_probe_attached((struct probe *)&s.btb_probe)) {
		probe_generic_deactivate((struct probe *)&s.btb_probe);
	}
}

unsigned int scope_prepare(unsigned int max_samples)
{
	unsigned int cnt = 0;
	struct scope_sample *samp;
	struct scope_sample_description d;
	size_t sz;

	// Remove any existing samples so we have a clean slate to work with.
	scope_flush();

	// Get the description of the scope sample so we know how many
	// bytes to allocate.
	scope_sample_desc(&d);
	sz = sizeof(struct scope_sample) - 1 + d.total_size;

	while (cnt < max_samples) {
		samp = (struct scope_sample *)kmalloc(sz, GFP_KERNEL);
		if (samp == NULL)
			break;
		// Memset so entire structure is paged in
		memset(samp, 0, sz);
		samp->scope = &s;
		samp->collected = false;
		list_add_tail(&samp->list, &prepared_samples);
		cnt++;
	}

	DEBUG("Successfully prepared %u samples.", cnt);
	return cnt;
}

unsigned int scope_collect(unsigned int delay, unsigned int timeout)
{
	struct list_head *cur, *next;
	struct scope_sample *samp;
	unsigned int cnt = 0;
	int cpu = s.target_cpu;

	DEBUG("Starting to collect.");
	if (list_empty(&prepared_samples))
		return 0;

	samp = list_first_entry(&prepared_samples, struct scope_sample, list);
	while (!samp->collected) {
		if (timeout-- == 0) {
			DEBUG("Timeout.");
			return 0;
		}
		smp_call_function_single(cpu, probes_collect, samp, true);
		ndelay(delay);
	}

	list_for_each_safe(cur, next, &prepared_samples) {
		samp = list_entry(cur, struct scope_sample, list);
		smp_call_function_single(cpu, probes_collect, samp, true);
		if (samp->collected) {
			list_del(cur);
			list_add_tail(cur, &collected_samples);
			cnt++;
			ndelay(delay);
		} else {
			break;
		}
	}
	collected_count += cnt;
	DEBUG("Finished collecting.");
	return cnt;
}

void scope_flush()
{
	struct list_head *cur, *next;
	struct scope_sample *samp;
	DEBUG("Flushing scope samples.");

	list_for_each_safe(cur, next, &prepared_samples) {
		samp = list_entry(cur, struct scope_sample, list);
		list_del(cur);
		kfree(samp);
	}

	list_for_each_safe(cur, next, &collected_samples) {
		samp = list_entry(cur, struct scope_sample, list);
		list_del(cur);
		kfree(samp);
	}
	collected_count = 0;
}

void scope_retrieve(void *buf, size_t * len)
{
	size_t samp_size, remaining, written;
	struct scope_sample_description d;
	struct list_head *cur, *next;
	struct scope_sample *samp;
	u8 *cur_loc;

	if (buf == NULL || len == NULL)
		return;

	DEBUG("Attempting to write to buffer %p of length %zu\n", buf, *len);

	scope_sample_desc(&d);
	samp_size = d.total_size;
	remaining = *len;
	written = 0;
	cur_loc = (u8 *) buf;

	list_for_each_safe(cur, next, &collected_samples) {
		// Only copy data if there's room
		if (remaining < samp_size)
			break;

		// Copy data from list element
		samp = list_entry(cur, struct scope_sample, list);
		memcpy(cur_loc, samp->data, samp_size);
		cur_loc += samp_size;
		written += samp_size;
		remaining -= samp_size;

		// Remove sample
		list_del(cur);
		collected_count -= 1;
		kfree(samp);
	}
	*len = written;
}

void scope_sample_desc(struct scope_sample_description *desc)
{
	size_t offs = 0;

	if (desc == NULL)
		return;

	memset(desc, 0, sizeof(struct scope_sample_description));

	if (is_probe_attached(&s.l1d_probe.base)) {
		desc->l1d.offs = offs;
		desc->l1d.size = probe_generic_sample_size(&s.l1d_probe.base);
		offs += desc->l1d.size;
	}

	if (is_probe_attached(&s.l1i_probe.base)) {
		desc->l1i.offs = offs;
		desc->l1i.size = probe_generic_sample_size(&s.l1i_probe.base);
		offs += desc->l1i.size;
	}

	if (is_probe_attached(&s.btb_probe.base)) {
		desc->btb.offs = offs;
		desc->btb.size = probe_generic_sample_size(&s.btb_probe.base);
		offs += desc->btb.size;
	}

	desc->total_size = offs;
}

unsigned int scope_sample_count()
{
	return collected_count;
}
