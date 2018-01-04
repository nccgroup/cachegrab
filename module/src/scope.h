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

#ifndef SCOPE_H__
#define SCOPE_H__

#include "cachegrab.h"
#include "probe_types.h"

struct scope {
	bool activated;
	struct probe_l1d l1d_probe;
	struct probe_l1i l1i_probe;
	struct probe_btb btb_probe;
	int target_cpu;
	bool created;
};

struct scope_configuration {
	bool created;
	int target_cpu;
	bool l1d_attached;
	bool l1i_attached;
	bool btb_attached;
};

struct scope_sample {
	struct list_head list;
	bool collected;
	struct scope *scope;
	size_t data_offs;
	u8 data[1];		// Variable length data
};

struct field {
	size_t offs;
	size_t size;
};

struct scope_sample_description {
	size_t total_size;
	struct field l1d;
	struct field l1i;
	struct field btb;
};

/**
 * Initialize the scope.
 *
 * @return 0 on success, <0 otherwise.
 */
int scope_init(void);

/**
 * Clean up the scope.
 */
void scope_term(void);

/**
 * Get the current configuration of the scope.
 */
void scope_get_config(struct scope_configuration *cfg);

/**
 * Create a new scope to monitor a target core.
 *
 * @param target_cpu The ID of the core to monitor.
 * @return CG_OK on success, error otherwise.
 */
enum CGState scope_create(int target_cpu);

/**
 * Destroy any existing scopes.
 *
 * Cleans up any resources in the process.
 */
void scope_destroy(void);

/**
 * "Attaches" one of the probes to the scope.
 *
 * This creates a probe for the specified cache architecture. It is
 * responsible for allocating any memory needed until the probe is detached.
 *
 * @param type Which probe type to attach.
 * @param shape The shape of the cache being attached to.
 * @return CG_OK if successful, error otherwise.
 */
enum CGState scope_attach_probe(enum probe_type t, struct cache_shape *sh);

/**
 * "Detaches" one of the probes from the scope.
 *
 * This takes the specified probe and cleans up any structures created during
 * probe_attach.
 *
 * @param type Which probe type to attach.
 */
void scope_detach_probe(enum probe_type type);

/**
 * Gets the scope by the specified type.
 *
 * @return A pointer to the probe of the given type if one is attached,
 *         otherwise NULL.
 */
struct probe *scope_get_probe(enum probe_type type);

/**
 * Configures one of the probes on the scope.
 *
 * @param type The type of probe to configure.
 * @param cfg The configuration to apply.
 * @return 0 if successful, -1 otherwise.
 */
int scope_configure_probe(enum probe_type type, struct probe_config *cfg);

/**
 * Activates all attached probes on the scope so collection begins.
 *
 * @return 0 if successful, -1 otherwise.
 */
int scope_activate(void);

/**
 * Deactivates all attached probes on the scope so collection stops.
 */
void scope_deactivate(void);

/**
 * Prepare the scope for collection.
 *
 * This allocates space needed for the samples. This gives us better performance
 * than allocating on the fly, since we don't have to wait for page faults or
 * the allocator. The return value is at most MAX_SAMPLES.
 *
 * @param max_samples The number of samples to try to allocate.
 * @return The number of empty samples successfully allocated.
 */
unsigned int scope_prepare(unsigned int max_samples);

/**
 * Arms the scope to begin the collection process.
 *
 * This function begins sending interrupts to the target core to execute the
 * probe function. Once the probes are activated, collection begins and
 * continues until the preallocated samples are all filled or the probes are
 * deactivated.
 *
 * @param delay The approximate delay in ns to wait between samples.
 * @param timeout The maximum number of unactivated samples before returning.
 * @return The number of samples that were collected.
 */
unsigned int scope_collect(unsigned int delay, unsigned int timeout);

/**
 * Flush all samples from the scope.
 *
 * Deallocates all samples on the scope. Some of them may contain collected
 * information and some may be prepared for collection.
 */
void scope_flush(void);

/**
 * Retrieves the collected samples and fills the specified buffer.
 *
 * This function assumes that BUF has already been verified to reside in user
 * land. Samples are written into the buffer and removed from kernel land in
 * a First-In-First-Out order. This function will only copy out samples which
 * have been filled by the probes.
 *
 * @param buf Buffer to fill with samples.
 * @param len Pointer to length of the specified buffer.
 */
void scope_retrieve(void *buf, size_t * len);

/**
 * Calculate information about the scope sample structure.
 *
 * The sizes and offsets of fields within this structure will change
 * during runtime.
 *
 * @param desc Pointer to the sample description structure. This will be
 *             filled in with values for field offsets and sizes.
 */
void scope_sample_desc(struct scope_sample_description *desc);

/**
 * Return the number of collected samples.
 *
 * @return Number of collected samples.
 */
unsigned int scope_sample_count(void);

#endif
