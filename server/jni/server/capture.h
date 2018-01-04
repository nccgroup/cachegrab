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

#ifndef CAPTURE_H__
#define CAPTURE_H__

#include "cachegrab.h"

#include <stdbool.h>
#include <stdlib.h>

/* The following section is usually included in sched.h, but not in Android.
 * We'll just define it on our own. */
#define CPU_SETSIZE 1024
#define __NCPUBITS  (8 * sizeof (unsigned long))
typedef struct
{
   unsigned long __bits[CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

#define CPU_SET(cpu, cpusetp) \
  ((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
  memset((cpusetp), 0, sizeof(cpu_set_t))
int sched_setaffinity(pid_t pid, size_t setsize, const cpu_set_t* set);
/* --- End of weird Android header hack --- */

struct capture_config {
  unsigned int max_samples;
  unsigned int stall_cutoff;
  unsigned int scope_time_delta;
  unsigned int scope_timeout;
  char* command;
  char* name;
  char* cbuf;
  bool debug;
};

struct shared_args {
  pthread_mutex_t status_lock;
  enum CGState status;

  pthread_mutex_t readiness_lock;
  pthread_cond_t all_ready;
  int ready_threads;
  volatile bool target_finished;
};

struct scope_args {
  int cpu;
  unsigned int max_samples;
  unsigned int time_delta;
  unsigned int timeout;
  unsigned int nsamples;
  struct shared_args *shared;
};

struct target_args {
  int cpu;
  char* command;
  char* name;
  char* cbuf;
  bool debug;
  uint8_t* out_stream;
  size_t out_len;
  uint8_t* err_stream;
  size_t err_len;
  int status;
  struct shared_args *shared;
};

struct stall_args {
  int cpu;
  unsigned int cutoff;
  struct shared_args *shared;
};

struct capture_output {
  int status;
  unsigned int nsamples;
  uint8_t* out_stream;
  size_t out_len;
  uint8_t* err_stream;
  size_t err_len;
};

void init_shared_args (struct shared_args* shared);
enum CGState get_shared_status (struct shared_args* shared);
void set_shared_status (struct shared_args* shared, enum CGState status);
void signal_ready (struct shared_args* shared);
bool set_cpu (int cpuid);
bool prioritize (void);

bool get_scope_args (struct scope_args *arg, struct capture_config *c, int cpu);
void* scope_func (void* p_arg);

bool get_stall_args (struct stall_args *arg, struct capture_config *c, int cpu);
void* stall_func (void* p_arg);

bool get_target_args (struct target_args *arg, struct capture_config *c, int cpu);
void* target_func (void* p_arg);

enum CGState capture (struct capture_config *cfg, struct capture_output *o);

#endif
