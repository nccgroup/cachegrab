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
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

void init_shared_args (struct shared_args* shared) {
  shared->ready_threads = 0;
  pthread_mutex_init(&shared->readiness_lock, NULL);
  pthread_mutex_init(&shared->status_lock, NULL);
  pthread_cond_init(&shared->all_ready, NULL);
  shared->status = CG_OK;
  shared->target_finished = false;
}

enum CGState get_shared_status (struct shared_args* shared) {
  enum CGState ret;
  pthread_mutex_lock(&shared->status_lock);
  ret = shared->status;
  pthread_mutex_unlock(&shared->status_lock);
  return ret;
}

void set_shared_status (struct shared_args* shared, enum CGState status) {
  pthread_mutex_lock(&shared->status_lock);
  shared->status = status;
  pthread_mutex_unlock(&shared->status_lock);
}

void signal_ready (struct shared_args* shared) {
  pthread_mutex_lock(&shared->readiness_lock);
  shared->ready_threads++;
  pthread_cond_wait(&shared->all_ready, &shared->readiness_lock);
  pthread_mutex_unlock(&shared->readiness_lock);
}

bool set_cpu (int cpuid) {
  cpu_set_t cset;

  CPU_ZERO(&cset);
  CPU_SET(cpuid, &cset);
  if (0 != sched_setaffinity(0, sizeof(cpu_set_t), &cset)) {
    return false;
  }
  return true;
}

bool prioritize () {
  struct sched_param param;
  param.sched_priority = sched_get_priority_max(SCHED_OTHER);
  sched_setparam(0, &param);
  return true;
}
