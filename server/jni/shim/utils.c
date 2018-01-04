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

#include "utils.h"

#include <string.h>
#include <stdio.h>

size_t hex_to_bytes(uint8_t* dst, char* src, size_t len) {
  size_t ctr = 0;
  size_t hexlen = strlen(src);
  size_t retval = 0;

  // Allow for implicit leading zero, e.g. 4AC -> 04AC
  if (hexlen % 2 == 1) {
    src--;
    hexlen++;
    ctr = 1;
  }

  uint8_t tmp = 0;
  while (ctr < hexlen && retval < len) {
    char c = src[ctr];
    uint8_t nibble;
    if ('0' <= c && c <= '9') {
      nibble = c - '0';
    } else if ('a' <= c && c <= 'f') {
      nibble = 10 + c - 'a';
    } else if ('A' <= c && c <= 'F') {
      nibble = 10 + c - 'A';
    } else {
      break;
    }
    if (ctr % 2 == 0) {
      tmp = (nibble & 0x0f) << 4;
    } else {
      dst[retval] = tmp | (nibble & 0x0f);
      retval++;
    }
    ctr++;
  }

  return retval;
}
