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

#ifndef ARM64_DEFS_H__
#define ARM64_DEFS_H__

// ARMv8 PMU events
/* See arch/arm64/kernel/perf_event.c */
#define ARMV8_IDX_TO_COUNTER(idx) ((idx) - 1)
#define EVENT_L1_ICACHE_REFILL 0x01
#define EVENT_L1_DCACHE_REFILL 0x03
#define EVENT_BRANCH_MISPRED   0x10

// ARMv8 Instructions
#define INS_SIZE 4
#define READ_EVCNTR 0xd53b9d41	/* mrs x1, pmxevcntr_el0 */
#define STORE_VALUE 0xf8008401	/* str x1, [x0],#8 */
#define NOP         0xd503201f	/* nop */
#define RET         0xd65f03c0	/* ret */
#define CMP_TRUE    0x6b01003f	/* cmp w1, w1 */
#define BEQ_4       0x54000020	/* b.eq #4 */

#endif
