/**
 * This file is part of the Cachegrab TEE library shim.
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

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

/**
 * Convert a hexadecimal string to a byte array.
 *
 * Writes at most LEN bytes to DST, and returns the number
 * of bytes written by the function.
 */
size_t hex_to_bytes(uint8_t* dst, char* src, size_t len);

#endif
