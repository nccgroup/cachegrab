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

#include "capture.h"

#include "scope.h"

bool get_scope_args (struct scope_args *arg, struct capture_config *c, int cpu) {
  arg->cpu = cpu;
  arg->max_samples = c->max_samples;
  arg->time_delta = c->scope_time_delta;
  arg->timeout = c->scope_timeout;
  return true;
}

void* scope_func (void* p_arg) {
  struct scope_args *arg = p_arg;

  if (!set_cpu(arg->cpu) ||
      !prioritize()) {
    set_shared_status(arg->shared, CG_CAPTURE_ERR);
  }
  // set up scope
  if (scope_prepare(arg->max_samples) == 0) {
    set_shared_status(arg->shared, CG_NO_MEM);
  }
  
  signal_ready(arg->shared);

  if (get_shared_status(arg->shared) == CG_OK) {
    // do scope things
    unsigned int collected_samples;
    collected_samples = scope_collect(arg->time_delta, arg->timeout);
    arg->nsamples = collected_samples;
  }
  return NULL;
}
