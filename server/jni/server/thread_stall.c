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

#include <stdio.h>

bool get_stall_args (struct stall_args *arg, struct capture_config *c, int cpu) {
  arg->cpu = cpu;
  arg->cutoff = c->stall_cutoff;
  return true;
}

void* stall_func (void* p_arg) {
  struct stall_args *arg = p_arg;

  if (!set_cpu(arg->cpu)) {
    set_shared_status(arg->shared, CG_CAPTURE_ERR);
  }
  signal_ready(arg->shared);

  if (get_shared_status(arg->shared) == CG_OK) {
    while (!arg->shared->target_finished && arg->cutoff--) {}
  }
  return NULL;
}
