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

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "capture_data.h"
#include "scope.h"

void get_capture_data (void) {
  struct capture_data* data;
  data = capture_data_retrieve();

  void* encoded_data;
  size_t encoded_len;
  
  encoded_data = capture_data_encode(&data->l1d_probe, &encoded_len);
  scope_set_probe_data(PROBE_TYPE_L1D, encoded_data, encoded_len);

  encoded_data = capture_data_encode(&data->l1i_probe, &encoded_len);
  scope_set_probe_data(PROBE_TYPE_L1I, encoded_data, encoded_len);

  encoded_data = capture_data_encode(&data->btb_probe, &encoded_len);
  scope_set_probe_data(PROBE_TYPE_BTB, encoded_data, encoded_len);

  capture_data_free(data);
}

enum CGState capture (struct capture_config *cfg, struct capture_output *o) {
  enum CGState ret = CG_CAPTURE_ERR;
  enum CGState err;
  int num_cores = sysconf( _SC_NPROCESSORS_ONLN );
  int init_cores = 0;
  pthread_t target_thread, scope_thread, *stall_threads;
  struct shared_args shared_args;
  struct scope_args scope_args;
  struct target_args target_args;
  struct stall_args *stall_args;
  struct scope *scope;
  int target_cpu, scope_cpu;
  bool successful = false;

  init_shared_args(&shared_args);
  
  err = scope_get_configuration(&scope);
  if (err != CG_OK) {
    ret = err;
    goto fail_get_config;
  }
  target_cpu = scope->target_cpu;
  scope_cpu = scope->scope_cpu;
  if (target_cpu == scope_cpu) {
    ret = CG_BAD_ARG;
    goto fail_get_config;
  }

  stall_threads = (pthread_t*)malloc(num_cores * sizeof(pthread_t));
  stall_args = (struct stall_args*)malloc(num_cores * sizeof(struct stall_args));
  if (stall_threads == NULL || stall_args == NULL) {
    ret = CG_NO_MEM;
    goto fail_stall_alloc;
  }
  
  if (!get_target_args(&target_args, cfg, target_cpu)) {
    ret = CG_BAD_ARG;
    goto fail_target_create;
  }
  target_args.shared = &shared_args;
  if (pthread_create(&target_thread, NULL, &target_func, &target_args) != 0)
    goto fail_target_create;

  if (!get_scope_args(&scope_args, cfg, scope_cpu)) {
    ret = CG_BAD_ARG;
    goto fail_scope_create;
  }
  scope_args.shared = &shared_args;
  if (pthread_create(&scope_thread, NULL, &scope_func, &scope_args) != 0)
    goto fail_scope_create;
  
  for (int i = 0; i < num_cores; i++) {
    if (i != target_cpu && i != scope_cpu) {
      if (!get_stall_args(&stall_args[i], cfg, i)) {
	ret = CG_BAD_ARG;
	goto fail_stall_create;
      }
      stall_args[i].shared = &shared_args;
      if (pthread_create(&stall_threads[i], NULL, &stall_func, &stall_args[i]) != 0)
	goto fail_stall_create;
      init_cores++;
    }
  }

  while (shared_args.ready_threads < num_cores) {
    usleep(1000);
  }
  pthread_cond_broadcast(&shared_args.all_ready);
  successful = true;

 fail_stall_create:
  for (int i = 0; i < num_cores; i++) {
    if (i != target_cpu && i != scope_cpu && init_cores > 0) {
      pthread_join(stall_threads[i], NULL);
      init_cores--;
    }
  }  
  pthread_join(scope_thread, NULL);
 fail_scope_create:
  pthread_join(target_thread, NULL);

  o->nsamples = scope_args.nsamples;
  o->status = target_args.status;
  o->out_stream = target_args.out_stream;
  o->out_len = target_args.out_len;
  o->err_stream = target_args.err_stream;
  o->err_len = target_args.err_len;
 fail_target_create:
  if (stall_threads)
    free(stall_threads);
  if (stall_args)
    free(stall_args);
 fail_stall_alloc:
 fail_get_config:
  if (successful) {
    get_capture_data();
    ret = get_shared_status(&shared_args);
  }
  return ret;
}
