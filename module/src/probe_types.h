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

#ifndef PROBE_GENERIC_H__
#define PROBE_GENERIC_H__

#include <linux/kernel.h>
#include <linux/perf_event.h>

#include "memory.h"

/**
 * Determine the number of sets in the selected range.
 *
 * Handle the case where the buffer wraps around.
 */
#define set_count(start, end, nset) ( \
		(start) < (end) ? (end) - (start) : (end) + (nset) - (start))

/**
 * Determine if a set falls within the given range.
 *
 * The buffer may wrap around, so we can't just do START <= SET <= END
 */
#define set_inside(set, start, end, nset) ( \
		((start) < (end) && (start) <= (set) && (set) < (end)) || \
		((start) > (end) && ((start) <= (set) || (set) < (end))))

struct probe;
typedef void (*probe_func) (u64 *, struct probe *);

enum probe_type {
	PROBE_TYPE_UNKNOWN,
	PROBE_TYPE_L1D,
	PROBE_TYPE_L1I,
	PROBE_TYPE_BTB,
};

/**
 * Configuration of a probe.
 *
 * The configuration may be changed while the probe is attached.
 */
struct probe_config {
	unsigned int set_start;
	unsigned int set_end;
};

/**
 * Structure to describe the shape of an arbitrary cache.
 *
 * This may be the BTB, L1D, or L1I.
 */
struct cache_shape {
	unsigned int num_sets;
	unsigned int associativity;
	unsigned int line_size;
};

/**
 * Generic structure to represent any of the supported probes.
 *
 * This structure should have as few pointers as possible, so as to reduce any
 * potential cache effects.
 */
struct probe {
	u8 attached;
	u8 activated;
	int pmu_idx;

	struct cache_shape cache_shape;
	struct probe_config cfg;
	u64 *raw_buf;

	probe_func measure;
	probe_func refill;

	unsigned int set_offset;
	struct perf_event *pevent;
	enum probe_type type;
};

/**
 * Structure representing the L1D probe.
 */
struct probe_l1d {
	struct probe base;
	u8 *kernel_code_base;
};

/**
 * Structure representing the L1I probe.
 */
struct probe_l1i {
	struct probe base;
	struct cgmem exec_mem;
};

/**
 * Structure representing the BTB probe.
 */
struct probe_btb {
	struct probe base;
	struct cgmem exec_mem;
};

/**
 * Structure to describe parameters needed to attach a probe.
 */
struct probe_params {
	int target_cpu;
	u64 type;
	u64 conf;
	struct cache_shape *cache_shape;
};

/**
 * Initialize the shared probe structure.
 */
void probe_generic_init(struct probe *p);
void probe_l1d_init(struct probe_l1d *p);
void probe_l1i_init(struct probe_l1i *p);
void probe_btb_init(struct probe_btb *p);

/**
 * Check if the probe is attached.
 *
 * @return 1 if attached, 0 otherwise.
 */
int is_probe_attached(struct probe *p);

/**
 * Attempt to enable the probing capability on the target core.
 *
 * This attempts to capture the performance counters on the target core.
 * This does not use the existing interfaces to the PMU because the
 * probe functions are hardcoded to expect.
 *
 * @param p The probe structure to enable.
 * @param target_cpu The target core to monitor.
 * @return CG_OK if successful, error otherwise.
 */
enum CGState probe_generic_attach(struct probe *p, struct probe_params *params);
enum CGState probe_l1d_attach(struct probe_l1d *p, int cpu,
			      struct cache_shape *s);
enum CGState probe_l1i_attach(struct probe_l1i *p, int cpu,
			      struct cache_shape *s);
enum CGState probe_btb_attach(struct probe_btb *p, int cpu,
			      struct cache_shape *s);

/**
 * Disable the probing capability on the target core.
 *
 * This releases the performance counters on the target core.
 *
 * @param p the probe structure to detach
 */
void probe_generic_detach(struct probe *p);
void probe_l1d_detach(struct probe_l1d *p);
void probe_l1i_detach(struct probe_l1i *p);
void probe_btb_detach(struct probe_btb *p);

/**
 * Configure the specified probe to only collect between the specified sets.
 *
 * This can give potentially better collection times.
 *
 * @param p The probe structure to configure.
 * @param cfg The desired configuration of the probe.
 * @return 0 if successful, -1 otherwise.
 */
int probe_generic_configure(struct probe *p, struct probe_config *cfg);
int probe_l1d_configure(struct probe_l1d *p, struct probe_config *cfg);
int probe_l1i_configure(struct probe_l1i *p, struct probe_config *cfg);
int probe_btb_configure(struct probe_btb *p, struct probe_config *cfg);

/**
 * Get the cache shape of a probe.
 *
 * @return A pointer to the probe cache shape.
 */
struct cache_shape *probe_generic_get_cache_shape(struct probe *p);

/**
 * Get the configuration of a probe.
 *
 * @return A pointer to the probe configuration.
 */
struct probe_config *probe_generic_get_config(struct probe *p);

/**
 * Check if the probe is activated.
 *
 * @return 1 if activated, 0 otherwise.
 */
int is_probe_activated(struct probe *p);

/**
 * Activate the specified probe.
 *
 * This resets and enables the counter associated with this probe.
 *
 * @param p the probe structure to activate
 * @return 0 if the activation was successful, -1 otherwise.
 */
int probe_generic_activate(struct probe *p);

/**
 * Deactivate the specified probe.
 *
 * This disables the counter associated with this probe.
 *
 * @param p The probe structure to deactivate.
 */
void probe_generic_deactivate(struct probe *p);

/**
 * Return the size of a sample from a given probe.
 *
 * @param p The probe object.
 * @return The size, in bytes, required by a sample from the probe.
 */
size_t probe_generic_sample_size(struct probe *p);

/**
 * Measure the desired property.
 *
 * These functions are called from probes_collect.
 *
 * @param buf Buffer to receive raw unprocessed data. The function assumes
 *            the buffer is large enough.
 * @param p Probe object that contains information about cache shape.
 */
void probe_l1d_measure(u64 * buf, struct probe_l1d *p);
void probe_l1i_measure(u64 * buf, struct probe_l1i *p);
void probe_btb_measure(u64 * buf, struct probe_btb *p);

/**
 * Refill the desired property.
 *
 * These functions perform the "Prime" part of Prime+Probe.
 *
 * @param buf Buffer which could receive raw unprocessed data. This is
 *            included so that the measure and refill functions could
 *            be interchangeable.
 * @param p Probe object that contains information about cache shape.
 */
void probe_l1d_refill(u64 * buf, struct probe_l1d *p);
void probe_l1i_refill(u64 * buf, struct probe_l1i *p);
void probe_btb_refill(u64 * buf, struct probe_btb *p);

#endif
