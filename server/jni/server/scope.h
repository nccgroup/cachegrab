/**
 * This file is part of the Cachegrab server.
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

#include <stdbool.h>
#include <stdint.h>

#include "cachegrab.h"

enum probe_type {
  PROBE_TYPE_L1D,
  PROBE_TYPE_L1I,
  PROBE_TYPE_BTB
};

struct collected_data {
  bool exists;
  void* buf;
  size_t len;
};

struct cache_shape {
  unsigned int num_sets;
  unsigned int associativity;
  unsigned int line_size;
};

struct probe_config {
  unsigned int set_start;
  unsigned int set_end;
};

struct probe {
  bool attached;
  enum probe_type type;
  struct cache_shape shape;
  struct probe_config cfg;
  struct scope* s;
  struct collected_data data;
};

struct scope {
  int driver_fd;

  bool connected;
  int target_cpu;
  int scope_cpu;
  
  struct probe l1d;
  struct probe l1i;
  struct probe btb;
};

// Duplicate of arg_scope_sample_desc because they're the same for now,
// but if either ever changes it makes more sense to have them separate.
struct field {
  size_t offs;
  size_t size;
};
struct scope_sample_desc {
  size_t total_size;
  struct field l1d;
  struct field l1i;
  struct field btb;
};

/**
 * Attempt to initialize the scope subsystem.
 *
 * @return 0 on success, < 0 on failure.
 */
int scope_init (void);

/**
 * Terminate the scope subsystem.
 */
void scope_term (void);

/**
 * Gets the configuration of a probe on the scope.
 *
 * @return A pointer to the probe structure.
 */
enum CGState scope_get_probe_configuration (enum probe_type type, struct probe** probe);

/**
 * Sets the probe configuration.
 */
void scope_set_probe_configuration (enum probe_type t, unsigned int start, unsigned int end);

/**
 * Gets the configuration of the scope.
 *
 * @return A pointer to the scope structure.
 */
enum CGState scope_get_configuration (struct scope** scope);

/**
 * Store the encoded data with the probe.
 */
enum CGState scope_set_probe_data (enum probe_type type, void* buf, size_t len);

/**
 * Retrieve the encoded data from the probe.
 */
enum CGState scope_get_probe_data (enum probe_type type, void** buf, size_t *len);

/**
 * Connect to the scope.
 *
 * @param cpu The cpu the target code will run on.
 * @return CG_OK if successful, error otherwise
 */
enum CGState scope_connect (int target_cpu, int scope_cpu);

/**
 * Disconnect from the scope.
 */
void scope_disconnect (void);

/**
 * Check if scope is connected.
 */
bool is_scope_connected (void);

/**
 * Attach a probe to the scope.
 *
 * @return CG_OK if successful, error otherwise.
 */
enum CGState scope_attach_probe (enum probe_type type, struct cache_shape* shape);

/**
 * Detach a probe from the scope.
 */
void scope_detach_probe (enum probe_type type);

/**
 * Prepare the sample for collection.
 */
unsigned int scope_prepare (unsigned int max_samples);

/**
 * Collect from the scope.
 */
unsigned int scope_collect (unsigned int delay, unsigned int timeout);

/**
 * Activate the scope so collection can begin.
 *
 * This is analogous to raising a trigger line high to start capture.
 * Call this function from the target core before executing the sensitive
 * region.
 *
 * @return True if the scope was successfully activated.
 */
bool scope_activate (void);

/**
 * Deactivate the scope so collection ends.
 *
 * This is analogous to bringing the trigger line back low to end capture.
 */
void scope_deactivate (void);

/**
 * Get the offsets and sizes of fields in the sample.
 */
void scope_sample_desc (struct scope_sample_desc *desc);

/**
 * Get the number of collected samples in the kernel scope.
 *
 * @return The number of collected samples that need to be retrieved.
 */
unsigned int scope_sample_count (void);

/**
 * Retrieve the samples into the specified buffer.
 *
 * @param buf Buffer to receive data.
 * @param len Pointer to length of BUF. Returns length of data written.
 */
void scope_retrieve (void *buf, size_t *len);

#endif
