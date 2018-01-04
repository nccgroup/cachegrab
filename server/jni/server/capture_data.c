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

#include "capture_data.h"

#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scope.h"

struct capture_data* capture_data_retrieve () {
  struct capture_data* ret;
  struct scope_sample_desc desc;
  unsigned int nsamples;

  ret = (struct capture_data*)malloc(sizeof(struct capture_data));
  if (ret == NULL)
    return ret;
  memset(ret, 0, sizeof(struct capture_data));

  // Find out how much space we have to allocate
  scope_sample_desc(&desc);
  nsamples = scope_sample_count();

  // Allocate it
  uint8_t* tmp;
  size_t tmp_sz = nsamples * desc.total_size;
  tmp = (uint8_t*)malloc(tmp_sz);
  if (!tmp)
    goto fail;

  if (desc.l1d.size > 0) {
    ret->l1d_probe.data = (uint8_t*)malloc(nsamples * desc.l1d.size);
    if (!ret->l1d_probe.data)
      goto fail;
    ret->l1d_probe.collected = true;
    ret->l1d_probe.sample_count = nsamples;
    ret->l1d_probe.sample_width = desc.l1d.size;
  }

  if (desc.l1i.size > 0) {
    ret->l1i_probe.data = (uint8_t*)malloc(nsamples * desc.l1i.size);
    if (!ret->l1i_probe.data)
      goto fail;
    ret->l1i_probe.collected = true;
    ret->l1i_probe.sample_count = nsamples;
    ret->l1i_probe.sample_width = desc.l1i.size;
  }

  if (desc.btb.size > 0) {
    ret->btb_probe.data = (uint8_t*)malloc(nsamples * desc.btb.size);
    if (!ret->btb_probe.data)
      goto fail;
    ret->btb_probe.collected = true;
    ret->btb_probe.sample_count = nsamples;
    ret->btb_probe.sample_width = desc.btb.size;
  }

  // Copy from kernel
  scope_retrieve(tmp, &tmp_sz);
  nsamples = tmp_sz / desc.total_size;

  // Fill in capture data structure
  for (unsigned int i = 0; i < nsamples; i++) {
    uint8_t *src, *dst;

    // Copy l1d data
    if (ret->l1d_probe.collected) {
      src = &tmp[i * desc.total_size + desc.l1d.offs];
      dst = &ret->l1d_probe.data[i * desc.l1d.size];
      memcpy(dst, src, desc.l1d.size);
    }

    // Copy l1i data
    if (ret->l1i_probe.collected) {
      src = &tmp[i * desc.total_size + desc.l1i.offs];
      dst = &ret->l1i_probe.data[i * desc.l1i.size];
      memcpy(dst, src, desc.l1i.size);
    }

    // Copy btb data
    if (ret->btb_probe.collected) {
      src = &tmp[i * desc.total_size + desc.btb.offs];
      dst = &ret->btb_probe.data[i * desc.btb.size];
      memcpy(dst, src, desc.btb.size);
    }
  }

  free(tmp);
  return ret;
 fail:
  if (tmp)
    free(tmp);
  if (ret->l1d_probe.data)
    free(ret->l1d_probe.data);
  if (ret->l1i_probe.data)
    free(ret->l1i_probe.data);
  if (ret->btb_probe.data)
    free(ret->btb_probe.data);
  free(ret);
  return NULL;
}

void write_to_enc_buffer (png_structp png, png_bytep data, png_size_t len) {
  struct enc_buffer *enc = png_get_io_ptr(png);

  if (enc->buf == NULL) {
    enc->buf = malloc(enc->len + len);
  } else {
    enc->buf = realloc(enc->buf, enc->len + len);
  }
  if (enc->buf) {
    memcpy(enc->buf + enc->len, data, len);
    enc->len += len;
  } else {
    png_error(png, "Error allocating memory");
  }
}

void* capture_data_encode (struct probe_data* d, size_t *len) {
  png_structp png = NULL;
  png_infop info = NULL;
  png_bytepp rows = NULL;
  png_bytep data = NULL;
  size_t data_sz;
  struct enc_buffer enc = {NULL, 0};
  void *ret = NULL;

  if (len)
    *len = 0;
  
  if (d == NULL || len == NULL || !d->collected || d->sample_count == 0)
    goto end;
  
  png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
    goto end;

  info = png_create_info_struct(png);
  if (info == NULL)
    goto end;

  if (setjmp(png_jmpbuf(png)))
    goto end;

  png_set_write_fn(png, (void*)&enc, write_to_enc_buffer, NULL);

  png_set_IHDR(png, info,
	       (uint32_t)d->sample_width, (uint32_t)d->sample_count,
	       8,
	       PNG_COLOR_TYPE_GRAY,
	       PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png, info);

  data_sz = d->sample_width * d->sample_count;
  data = (png_bytep)malloc(data_sz);
  if (data == NULL)
    goto end;

  rows = (png_bytepp)malloc(d->sample_count * sizeof(png_bytep));
  if (rows == NULL)
    goto end;

  memcpy(data, d->data, data_sz);
  for (unsigned int i = 0; i < d->sample_count; i++) {
    rows[i] = data + (d->sample_width * i);
  }

  png_write_image(png, rows);
  png_write_end(png, info);

  ret = enc.buf;
  *len = enc.len;
  enc.buf = NULL;
 end:
  if (enc.buf)
    free(enc.buf);
  if (rows)
    free(rows);
  if (data)
    free(data);
  if (info)
    png_destroy_info_struct(png, &info);
  if (png)
    png_destroy_write_struct(&png, NULL);
  return ret;
}

void capture_data_free (struct capture_data* data) {
  if (data == NULL)
    return;

  if (data->l1d_probe.collected)
    free(data->l1d_probe.data);

  if (data->l1i_probe.collected)
    free(data->l1i_probe.data);

  if (data->btb_probe.collected)
    free(data->btb_probe.data);

  free(data);
}

