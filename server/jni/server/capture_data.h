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

#ifndef CAPTURE_DATA_H__
#define CAPTURE_DATA_H__
#include <stdbool.h>
#include <stdint.h>

struct enc_buffer {
  uint8_t *buf;
  size_t len;
};

struct probe_data {
  bool collected;
  uint8_t *data;
  unsigned int sample_count;
  size_t sample_width;
};

struct capture_data {
  struct probe_data l1d_probe;
  struct probe_data l1i_probe;
  struct probe_data btb_probe;
};

/**
 * Retrieve data from the kernel land scope.
 *
 * Take the data from the capture and store it in the provided capture_data
 * structure. The returned pointer must be freed by capture_data_free.
 *
 * @return Null in case of error, otherwise a pointer to a newly created
 *         capture_data structure.
 */
struct capture_data* capture_data_retrieve (void);

/**
 * Encode the capture data to a PNG.
 *
 * @return NULL on failure, otherwise pointer to PNG buffer, with length
 *         in #len. Caller must free.
 */
void* capture_data_encode (struct probe_data *d, size_t *len);

/**
 * Free the previously allocated capture_data structure.
 */
void capture_data_free (struct capture_data* data);

#endif
