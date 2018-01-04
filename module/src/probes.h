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

#ifndef PROBES_H__
#define PROBES_H__

/**
 * Initialize all structures needed for the probes.
 *
 * @return 0 if successful, -1 otherwise.
 */
int probes_init(void);

/**
 * Cleans up all structures related to probes.
 */
void probes_term(void);

/**
 * Function run on the target core.
 */
void probes_collect(void *p);

#endif
