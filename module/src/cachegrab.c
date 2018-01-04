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

#include "cachegrab.h"

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "cachegrab_ioctl.h"
#include "probes.h"
#include "scope.h"

MODULE_AUTHOR("Keegan Ryan");
MODULE_LICENSE("GPL");

static unsigned int cachegrab_major = 0;
static struct cdev cachegrab_cdev;
static struct class *cachegrab_class = NULL;

struct file_operations fops = {
	.unlocked_ioctl = cachegrab_ioctl,
};

int init_components(void)
{
	int err;
	err = probes_init();
	if (err < 0) {
		ERR("Could not initialize probes.");
		goto fail_probe;
	}

	err = scope_init();
	if (err < 0) {
		ERR("Could not initialize scope.");
		goto fail_scope;
	}
	return 0;
 fail_scope:
	probes_term();
 fail_probe:
	return err;
}

void term_components(void)
{
	scope_term();
	probes_term();
}

/**
 * Set up module and create /dev/cachegrab file.
 */
int init_module()
{
	int err;
	dev_t dev;
	struct device *device = NULL;

	err = init_components();
	if (err < 0) {
		ERR("Failed to initialize components.");
		return err;
	}

	err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (err < 0) {
		ERR("Failed to allocate chrdev region.");
		return err;
	}
	cachegrab_major = MAJOR(dev);
	DEBUG("Major number is %d", cachegrab_major);

	cachegrab_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(cachegrab_class)) {
		ERR("Failed to create class.");
		err = PTR_ERR(cachegrab_class);
		goto bad;
	}

	cdev_init(&cachegrab_cdev, &fops);
	err = cdev_add(&cachegrab_cdev, dev, 1);
	if (err < 0) {
		WARNING("Failed to add character device.");
		goto bad;
	}

	device = device_create(cachegrab_class, NULL, dev, NULL, DEVICE_NAME);
	if (IS_ERR(device)) {
		ERR("Failed to create device");
		err = PTR_ERR(device);
		goto bad;
	}

	DEBUG("Ready to go");

	return 0;
 bad:
	if (cachegrab_class)
		class_destroy(cachegrab_class);

	cdev_del(&cachegrab_cdev);
	unregister_chrdev_region(dev, 1);
	return err;
}

void cleanup_module()
{
	dev_t devno = MKDEV(cachegrab_major, 0);
	device_destroy(cachegrab_class, devno);
	class_destroy(cachegrab_class);
	cdev_del(&cachegrab_cdev);
	unregister_chrdev_region(devno, 1);

	term_components();
}
