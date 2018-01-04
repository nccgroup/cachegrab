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

#include "cachegrab_ioctl.h"

#include <asm/uaccess.h>

#include "cachegrab.h"
#include "scope.h"
#include "probe_types.h"

enum probe_type get_probe_type(enum arg_probe_type p)
{
	switch (p) {
	case ARG_PROBE_TYPE_L1D:
		return PROBE_TYPE_L1D;
	case ARG_PROBE_TYPE_L1I:
		return PROBE_TYPE_L1I;
	case ARG_PROBE_TYPE_BTB:
		return PROBE_TYPE_BTB;
	default:
		return PROBE_TYPE_UNKNOWN;
	}
}

long probe_attach_ioctl(void __user * p)
{
	struct arg_probe_attach arg;
	enum probe_type type;
	struct cache_shape shape;

	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	type = get_probe_type(arg.type);
	shape.num_sets = arg.num_sets;
	shape.associativity = arg.associativity;
	shape.line_size = arg.line_size;

	return (long)scope_attach_probe(type, &shape);
}

long probe_detach_ioctl(void __user * p)
{
	enum probe_type type;
	struct arg_probe_configure arg;
	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	type = get_probe_type(arg.type);
	scope_detach_probe(type);
	return CG_OK;
}

long probe_get_config_ioctl(void __user * p)
{
	struct arg_probe_get_config arg;
	struct probe *pr;
	enum CGState ret;

	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	pr = scope_get_probe(get_probe_type(arg.type));
	if (pr != NULL) {
		struct cache_shape *cs = probe_generic_get_cache_shape(pr);
		struct probe_config *cfg = probe_generic_get_config(pr);

		arg.attached = true;
		arg.num_sets = cs->num_sets;
		arg.associativity = cs->associativity;
		arg.line_size = cs->line_size;
		arg.set_start = cfg->set_start;
		arg.set_end = cfg->set_end;
		ret = CG_OK;
	} else {
		arg.attached = false;
		arg.num_sets = 0;
		arg.associativity = 0;
		arg.line_size = 0;
		arg.set_start = 0;
		arg.set_end = 0;
		ret = CG_PROBE_NOT_CONNECTED;
	}

	if (copy_to_user(p, &arg, sizeof(arg)) != 0)
		return CG_PERM;

	return ret;
}

long probe_configure_ioctl(void __user * p)
{
	int err;
	enum probe_type type;
	struct probe_config cfg;

	struct arg_probe_configure arg;
	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	type = get_probe_type(arg.type);
	cfg.set_start = arg.set_start;
	cfg.set_end = arg.set_end;
	err = scope_configure_probe(type, &cfg);

	return (err != 0) ? CG_PERM : CG_OK;
}

long scope_config_ioctl(void __user * p)
{
	struct arg_scope_config arg_config;
	struct scope_configuration config;

	scope_get_config(&config);
	arg_config.created = config.created;
	arg_config.target_cpu = config.target_cpu;
	arg_config.l1d_attached = config.l1d_attached;
	arg_config.l1i_attached = config.l1i_attached;
	arg_config.btb_attached = config.btb_attached;

	if (copy_to_user(p, &arg_config, sizeof(arg_config)) != 0)
		return CG_PERM;

	return CG_OK;
}

long scope_create_ioctl(void __user * p)
{
	struct arg_scope_create arg;
	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;
	return scope_create(arg.target_cpu);
}

long scope_destroy_ioctl(void)
{
	scope_destroy();
	return CG_OK;
}

long scope_activate_ioctl(void)
{
	return (long)scope_activate();
}

long scope_deactivate_ioctl(void)
{
	scope_deactivate();
	return CG_OK;
}

long scope_prepare_ioctl(unsigned long p)
{
	unsigned int max_samples = (unsigned int)p;
	unsigned int created_samples;

	// Limit the maximum number of samples so that the number of created
	// samples will fit inside of a signed long.
	if (max_samples > INT_MAX)
		max_samples = (unsigned int)INT_MAX;

	created_samples = scope_prepare(max_samples);
	return (long)created_samples;
}

long scope_collect_ioctl(void __user * p)
{
	struct arg_scope_collect arg;
	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	return (long)scope_collect(arg.delay, arg.timeout);
}

long scope_flush_ioctl(void)
{
	scope_flush();
	return CG_OK;
}

long scope_retrieve_ioctl(void __user * p)
{
	struct arg_scope_retrieve arg;
	if (copy_from_user(&arg, p, sizeof(arg)) != 0)
		return CG_PERM;

	if (!access_ok(VERIFY_WRITE, arg.buf, arg.len))
		return CG_PERM;

	scope_retrieve(arg.buf, &arg.len);

	if (copy_to_user(p, &arg, sizeof(arg)) != 0)
		return CG_PERM;

	return CG_OK;
}

long scope_sample_desc_ioctl(void __user * p)
{
	struct arg_scope_sample_desc arg_desc;
	struct scope_sample_description internal_desc;

	scope_sample_desc(&internal_desc);
	arg_desc.total_size = internal_desc.total_size;
	arg_desc.l1d.offs = internal_desc.l1d.offs;
	arg_desc.l1d.size = internal_desc.l1d.size;
	arg_desc.l1i.offs = internal_desc.l1i.offs;
	arg_desc.l1i.size = internal_desc.l1i.size;
	arg_desc.btb.offs = internal_desc.btb.offs;
	arg_desc.btb.size = internal_desc.btb.size;

	if (copy_to_user(p, &arg_desc, sizeof(arg_desc)) != 0)
		return CG_PERM;

	return CG_OK;
}

long scope_sample_count_ioctl(void)
{
	return (long)scope_sample_count();
}

long cachegrab_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case CG_PROBE_ATTACH:
		return probe_attach_ioctl((void __user *)arg);
	case CG_PROBE_DETACH:
		return probe_detach_ioctl((void __user *)arg);
	case CG_PROBE_GET_CONFIG:
		return probe_get_config_ioctl((void __user *)arg);
	case CG_PROBE_CONFIGURE:
		return probe_configure_ioctl((void __user *)arg);

	case CG_SCOPE_GET_CONFIG:
		return scope_config_ioctl((void __user *)arg);
	case CG_SCOPE_CREATE:
		return scope_create_ioctl((void __user *)arg);
	case CG_SCOPE_DESTROY:
		return scope_destroy_ioctl();
	case CG_SCOPE_ACTIVATE:
		return scope_activate_ioctl();
	case CG_SCOPE_DEACTIVATE:
		return scope_deactivate_ioctl();
	case CG_SCOPE_PREPARE:
		return scope_prepare_ioctl(arg);
	case CG_SCOPE_COLLECT:
		return scope_collect_ioctl((void __user *)arg);
	case CG_SCOPE_FLUSH:
		return scope_flush_ioctl();
	case CG_SCOPE_RETRIEVE:
		return scope_retrieve_ioctl((void __user *)arg);
	case CG_SCOPE_SAMPLE_DESC:
		return scope_sample_desc_ioctl((void __user *)arg);
	case CG_SCOPE_SAMPLE_COUNT:
		return scope_sample_count_ioctl();
	default:
		return CG_BAD_CMD;
	}
}
