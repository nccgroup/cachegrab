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

#include "scope.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "driver.h"

static struct scope s;

int scope_init () {
  memset(&s, 0, sizeof(s));
  s.connected = false;
  s.l1d.type = PROBE_TYPE_L1D;
  s.l1d.s = &s;
  s.l1d.data.exists = false;
  
  s.l1i.type = PROBE_TYPE_L1I;
  s.l1i.s = &s;
  s.l1i.data.exists = false;

  s.btb.type = PROBE_TYPE_BTB;
  s.btb.s = &s;
  s.btb.data.exists = false;

  s.driver_fd = open(DEVFILE, O_RDWR);
  if (s.driver_fd < 0) {
    return -1;
  }
  
  return 0;
}

void scope_term () {
  close(s.driver_fd);
}

enum CGState scope_get_probe_configuration (enum probe_type type, struct probe** probe) {
  struct arg_probe_get_config cfg;
  struct probe* p;
  enum CGState ret;
  
  switch (type) {
  case PROBE_TYPE_L1D:
    cfg.type = ARG_PROBE_TYPE_L1D;
    p = &s.l1d;
    break;
  case PROBE_TYPE_L1I:
    cfg.type = ARG_PROBE_TYPE_L1I;
    p = &s.l1i;
    break;
  case PROBE_TYPE_BTB:
    cfg.type = ARG_PROBE_TYPE_BTB;
    p = &s.btb;
    break;
  default:
    return CG_BAD_ARG;
  }
  ret = ioctl(s.driver_fd, CG_PROBE_GET_CONFIG, &cfg);
  if (ret != CG_OK && ret != CG_PROBE_NOT_CONNECTED)
    return ret;

  if (cfg.attached) {
    p->attached = true;
    
    p->shape.num_sets = cfg.num_sets;
    p->shape.associativity = cfg.associativity;
    p->shape.line_size = cfg.line_size;

    p->cfg.set_start = cfg.set_start;
    p->cfg.set_end = cfg.set_end;    
  } else {
    p->attached = false;
  }
  if (probe != NULL)
    *probe = p;
  return CG_OK;
}

enum CGState scope_get_configuration (struct scope** scope) {
  struct arg_scope_config cfg;
  enum CGState ret;

  ret = ioctl(s.driver_fd, CG_SCOPE_GET_CONFIG, &cfg);
  if (ret != CG_OK)
    return ret;

  s.connected = cfg.created;
  s.target_cpu = cfg.target_cpu;
  ret = scope_get_probe_configuration(PROBE_TYPE_L1D, NULL);
  if (ret != CG_OK)
    return ret;

  ret = scope_get_probe_configuration(PROBE_TYPE_L1I, NULL);
  if (ret != CG_OK)
    return ret;

  ret = scope_get_probe_configuration(PROBE_TYPE_BTB, NULL);
  if (ret != CG_OK)
    return ret;

  if (scope)
    *scope = &s;
  return CG_OK;
};

enum CGState scope_connect (int target_cpu, int scope_cpu) {
  struct arg_scope_create p;
  enum CGState ret;
  
  p.target_cpu = target_cpu;
  ret = ioctl(s.driver_fd, CG_SCOPE_CREATE, &p);

  if (ret == CG_OK) {
    s.connected = true;
    s.target_cpu = target_cpu;
    s.scope_cpu = scope_cpu;
  }
  return ret;
}

void scope_disconnect () {
  ioctl(s.driver_fd, CG_SCOPE_DESTROY, NULL);
  scope_get_configuration(NULL);
  scope_set_probe_data(PROBE_TYPE_L1D, NULL, 0);
  scope_set_probe_data(PROBE_TYPE_L1I, NULL, 0);
  scope_set_probe_data(PROBE_TYPE_BTB, NULL, 0);
}

bool is_scope_connected () {
  return s.connected;
}

enum CGState scope_attach_probe (enum probe_type type, struct cache_shape* shape) {
  struct arg_probe_attach arg;
  enum CGState ret = CG_BAD_ARG;

  switch (type) {
  case PROBE_TYPE_L1D:
    arg.type = ARG_PROBE_TYPE_L1D;
    break;
  case PROBE_TYPE_L1I:
    arg.type = ARG_PROBE_TYPE_L1I;
    break;
  case PROBE_TYPE_BTB:
    arg.type = ARG_PROBE_TYPE_BTB;
    break;
  default:
    return ret;
  }

  arg.num_sets = shape->num_sets;
  arg.associativity = shape->associativity;
  arg.line_size = shape->line_size;
  ret = ioctl(s.driver_fd, CG_PROBE_ATTACH, &arg);
  scope_get_configuration(NULL);
  return ret;
}

void scope_set_probe_configuration (enum probe_type t, unsigned int start, unsigned int end) {
  scope_set_probe_data(t, NULL, 0);
  
  struct arg_probe_configure arg;
  switch (t) {
  case PROBE_TYPE_L1D:
    arg.type = ARG_PROBE_TYPE_L1D;
    break;
  case PROBE_TYPE_L1I:
    arg.type = ARG_PROBE_TYPE_L1I;
    break;
  case PROBE_TYPE_BTB:
    arg.type = ARG_PROBE_TYPE_BTB;
    break;
  default:
    return;
  }
  
  arg.set_start = start;
  arg.set_end = end;

  ioctl(s.driver_fd, CG_PROBE_CONFIGURE, &arg);

  scope_get_configuration(NULL);
}

enum CGState scope_set_probe_data (enum probe_type type, void* buf, size_t len) {
  struct probe* p;

  switch (type) {
  case PROBE_TYPE_L1D:
    p = &s.l1d;
    break;
  case PROBE_TYPE_L1I:
    p = &s.l1i;
    break;
  case PROBE_TYPE_BTB:
    p = &s.btb;
    break;
  default:
    return CG_BAD_ARG;
  }

  if (p->data.exists) {
    free(p->data.buf);
  }
  if (buf) {
    p->data.exists = true;
    p->data.buf = buf;
    p->data.len = len;
  } else {
    p->data.exists = false;
  }
  return CG_OK;
}

enum CGState scope_get_probe_data (enum probe_type type, void** buf, size_t *len) {
  struct probe* p;
  
  if (buf == NULL || len == NULL)
    return CG_BAD_ARG;

  switch (type) {
  case PROBE_TYPE_L1D:
    p = &s.l1d;
    break;
  case PROBE_TYPE_L1I:
    p = &s.l1i;
    break;
  case PROBE_TYPE_BTB:
    p = &s.btb;
    break;
  default:
    return CG_BAD_ARG;
  }

  if (p->data.exists) {
    *buf = p->data.buf;
    *len = p->data.len;
  } else {
    *buf = NULL;
    *len = 0;
  }
  
  return CG_OK;
}


void scope_detach_probe (enum probe_type type) {
  struct arg_probe_detach arg;

  scope_set_probe_data(type, NULL, 0);
  switch (type) {
  case PROBE_TYPE_L1D:
    arg.type = ARG_PROBE_TYPE_L1D;
    break;
  case PROBE_TYPE_L1I:
    arg.type = ARG_PROBE_TYPE_L1I;
    break;
  case PROBE_TYPE_BTB:
    arg.type = ARG_PROBE_TYPE_BTB;
    break;
  default:
    return;
  }
  ioctl(s.driver_fd, CG_PROBE_DETACH, &arg);
  scope_get_configuration(NULL);
}

unsigned int scope_prepare (unsigned int max_samples) {
  unsigned int ret;
  if (max_samples != 0) {
    ret = ioctl(s.driver_fd, CG_SCOPE_PREPARE, (unsigned long)max_samples);
  } else {
    ioctl(s.driver_fd, CG_SCOPE_FLUSH, NULL);
    ret = 0;
  }
  return ret;
}

unsigned int scope_collect (unsigned int delay, unsigned int timeout) {
  struct arg_scope_collect arg = {
    .delay = delay,
    .timeout = timeout
  };
  unsigned int ret;
  ret = ioctl(s.driver_fd, CG_SCOPE_COLLECT, &arg);
  return ret;
}

bool scope_activate () {
  int ret = ioctl(s.driver_fd, CG_SCOPE_ACTIVATE, NULL);
  return (ret == 0);
}

void scope_deactivate () {
  ioctl(s.driver_fd, CG_SCOPE_DEACTIVATE, NULL);
}

void scope_sample_desc (struct scope_sample_desc *desc) {
  if (desc == NULL)
    return;
  
  struct arg_scope_sample_desc arg_desc;
  ioctl(s.driver_fd, CG_SCOPE_SAMPLE_DESC, &arg_desc);

  desc->total_size = arg_desc.total_size;
  desc->l1d.offs   = arg_desc.l1d.offs;
  desc->l1d.size   = arg_desc.l1d.size;
  desc->l1i.offs   = arg_desc.l1i.offs;
  desc->l1i.size   = arg_desc.l1i.size;
  desc->btb.offs   = arg_desc.btb.offs;
  desc->btb.size   = arg_desc.btb.size;
}

unsigned int scope_sample_count () {
  int ret;
  ret = ioctl(s.driver_fd, CG_SCOPE_SAMPLE_COUNT, NULL);
  if (ret < 0)
    return 0;
  else
    return (unsigned int) ret;
}

void scope_retrieve (void *buf, size_t *len) {
  int ret;
  struct arg_scope_retrieve p;

  if (len == NULL) {
    return;
  }
    
  p.buf = buf;
  p.len = *len;
  ret = ioctl(s.driver_fd, CG_SCOPE_RETRIEVE, &p);
  
  if (ret < 0)
    *len = 0;
  else
    *len = p.len;
}
