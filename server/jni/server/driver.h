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

#ifndef DRIVER_H__
#define DRIVER_H__

#define DEVFILE "/dev/cachegrab"

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdint.h>

enum arg_probe_type {
	ARG_PROBE_TYPE_L1D,
	ARG_PROBE_TYPE_L1I,
	ARG_PROBE_TYPE_BTB,
};

struct arg_probe_attach {
	enum arg_probe_type type;
	unsigned int num_sets;
	unsigned int associativity;
	unsigned int line_size;
};

struct arg_probe_detach {
	enum arg_probe_type type;
};

struct arg_probe_get_config {
	enum arg_probe_type type;
	bool attached;
	unsigned int num_sets;
	unsigned int associativity;
	unsigned int line_size;
	unsigned int set_start;
	unsigned int set_end;
};

struct arg_probe_configure {
	enum arg_probe_type type;
	unsigned int set_start;
	unsigned int set_end;
};

struct arg_scope_config {
	bool created;
	int target_cpu;
	bool l1d_attached;
	bool l1i_attached;
	bool btb_attached;
};

struct arg_scope_create {
	int target_cpu;
};

struct arg_scope_collect {
	unsigned int delay;
	unsigned int timeout;
};

struct arg_scope_retrieve {
	void *buf;
	size_t len;
};

struct arg_scope_sample_desc_field {
	size_t offs;
	size_t size;
};

struct arg_scope_sample_desc {
	size_t total_size;
	struct arg_scope_sample_desc_field l1d;
	struct arg_scope_sample_desc_field l1i;
	struct arg_scope_sample_desc_field btb;
};

#define CG_MAGIC 47
#define CG_PROBE_ATTACH _IOW(CG_MAGIC, 0x00, struct arg_probe_attach)
#define CG_PROBE_DETACH _IOW(CG_MAGIC, 0x01, struct arg_probe_detach)
#define CG_PROBE_GET_CONFIG _IOR(CG_MAGIC, 0x02, struct arg_probe_get_config)
#define CG_PROBE_CONFIGURE _IOW(CG_MAGIC, 0x03, struct arg_probe_configure)

#define CG_SCOPE_GET_CONFIG _IOR(CG_MAGIC, 0x10, struct arg_scope_config)
#define CG_SCOPE_CREATE _IOW(CG_MAGIC, 0x11, struct arg_scope_create)
#define CG_SCOPE_DESTROY _IO(CG_MAGIC, 0x12)
#define CG_SCOPE_ACTIVATE _IO(CG_MAGIC, 0x13)
#define CG_SCOPE_DEACTIVATE _IO(CG_MAGIC, 0x14)
#define CG_SCOPE_PREPARE _IO(CG_MAGIC, 0x15)
#define CG_SCOPE_COLLECT _IOW(CG_MAGIC, 0x16, struct arg_scope_collect)
#define CG_SCOPE_FLUSH _IO(CG_MAGIC, 0x17)
#define CG_SCOPE_RETRIEVE _IOR(CG_MAGIC, 0x18, struct arg_scope_retrieve)
#define CG_SCOPE_SAMPLE_DESC _IOR(CG_MAGIC, 0x19, struct arg_scope_sample_desc)
#define CG_SCOPE_SAMPLE_COUNT _IO(CG_MAGIC, 0x1A)

#endif
