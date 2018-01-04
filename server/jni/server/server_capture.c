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

#include "server_capture.h"

#include "capture.h"
#include "server.h"
#include "scope.h"

void free_capture_config (struct capture_config *cfg);

bool get_capture_config (struct capture_config *cfg, struct mg_str *ps, bool url) {
  char nsamp_s[10];
  char s_cut_s[10];
  char del_s[10];
  char to_s[10];
  char command[1024];
  char name[256];
  char cbuf[1024];
  char debug[10];

  unsigned int samples;
  unsigned int s_cut;
  unsigned int delta;
  unsigned int timeout;

  if (mg_get_http_var(ps, "max_samples", nsamp_s, sizeof(nsamp_s)) <= 0 ||
      1 != sscanf(nsamp_s, "%u", &samples) ||
      samples == 0)
    return false;

  if (mg_get_http_var(ps, "stalling_cutoff", s_cut_s, sizeof(s_cut_s)) <= 0 ||
      1 != sscanf(s_cut_s, "%d", &s_cut))
    s_cut = DEFAULT_STALL_CUTOFF;

  if (mg_get_http_var(ps, "time_delta", del_s, sizeof(del_s)) <= 0 ||
      1 != sscanf(del_s, "%u", &delta))
    delta = DEFAULT_DELTA;

  if (mg_get_http_var(ps, "timeout", to_s, sizeof(to_s)) <= 0 ||
      1 != sscanf(to_s, "%u", &timeout))
    timeout = DEFAULT_TIMEOUT;

  int len;
  
  if ((len = mg_get_http_var(ps, "command", command, sizeof(command))) <= 0)
    goto memerr;
  cfg->command = (char*)malloc(len + 1);
  if (!cfg->command)
    goto memerr;
  memcpy(cfg->command, command, len + 1);

  name[0] = '\0';
  if ((len = mg_get_http_var(ps, "ta_name", name, sizeof(name))) < 0)
    goto memerr;
  cfg->name = (char*)malloc(len + 1);
  if (!cfg->name)
    goto memerr;
  memcpy(cfg->name, name, len + 1);

  cbuf[0] = '\0';
  if ((len = mg_get_http_var(ps, "trigger_cbuf", cbuf, sizeof(cbuf))) < 0)
    goto memerr;
  cfg->cbuf = (char*)malloc(len + 1);
  if (!cfg->cbuf)
    goto memerr;
  memcpy(cfg->cbuf, cbuf, len + 1);

  if (mg_get_http_var(ps, "debug", debug, sizeof(debug)) > 0 &&
      0 == strcmp("y", debug)) {
    cfg->debug = true;
  } else {
    cfg->debug = false;
  }

  cfg->max_samples = samples;
  cfg->stall_cutoff = s_cut;
  cfg->scope_time_delta = delta;
  cfg->scope_timeout = timeout;
  return true;
 memerr:
  free_capture_config(cfg);
  return false;
}

void free_capture_config (struct capture_config *cfg) {
  if (cfg->command) {
    free(cfg->command);
    cfg->command = NULL;
  }
  if (cfg->name) {
    free(cfg->name);
    cfg->name = NULL;
  }
  if (cfg->cbuf) {
    free(cfg->cbuf);
    cfg->cbuf = NULL;
  }
}

void handle_capture (struct mg_connection *nc, int ev, void *data) {
  struct http_message *msg = data;
  struct mg_str *params;
  enum CGState err = CG_BAD_ARG;
  struct capture_config cfg = {0};
  struct capture_output o;
  bool is_get;
  o.out_stream = o.err_stream = NULL;
  
  if (0 == mg_strcmp(GET, msg->method)) {
    is_get = true;
  } else if (0 == mg_strcmp(POST, msg->method)) {
    is_get = false;
  } else {
    goto err;
  }

  if (is_get)
    params = &msg->query_string;
  else
    params = &msg->body;
    
  if (!get_capture_config(&cfg, params, is_get))
    goto err;
  err = capture(&cfg, &o);
  if (err != CG_OK)
    goto err;

  if (is_get) {
    struct scope *s;
    err = scope_get_configuration(&s);
    if (err != CG_OK)
      goto err;

    HTTP_OK(nc);
    mg_printf(nc, "<html><body><table><tr>");
    if (s->l1d.attached)
      mg_printf(nc, "<td>L1D<br><img src='l1d.png'><br></td>");
    if (s->l1i.attached)
      mg_printf(nc, "<td>L1I<br><img src='l1i.png'><br></td>");
    if (s->btb.attached)
      mg_printf(nc, "<td>BTB<br><img src='btb.png'><br></td>");
    mg_printf(nc, "</tr></table>");
    if (o.out_stream) {
      mg_printf(nc, "<b>STDOUT</b><br>");
      mg_send(nc, o.out_stream, o.out_len);
    }
    if (o.err_stream) {
      mg_printf(nc, "<b>STDERR</b><br>");
      mg_send(nc, o.err_stream, o.err_len);
    }
    mg_printf(nc, "</body></html>");
    HTTP_DONE(nc);
  } else {
    HTTP_OK(nc);
    mg_printf(nc, "{");
    print_status(nc, err);
    mg_printf(nc, ", ");

    mg_printf(nc, "\"num_samples\": %u, ", o.nsamples);
    mg_printf(nc, "\"return_code\": %d, ", o.status);

    mg_printf(nc, "\"stdout\": \"");
    if (o.out_stream) {
      print_buf(nc, o.out_stream, o.out_len);
    }
    mg_printf(nc, "\", ");

    mg_printf(nc, "\"stderr\": \"");
    if (o.err_stream) {
      print_buf(nc, o.err_stream, o.err_len);
    }
    mg_printf(nc, "\"");
    
    mg_printf(nc, "}");
    HTTP_DONE(nc);
  }

  if (o.out_stream)
    free(o.out_stream);
  if (o.err_stream)
    free(o.err_stream);
  free_capture_config(&cfg);
  return;
 err:
  if (o.out_stream)
    free(o.out_stream);
  if (o.err_stream)
    free(o.err_stream);
  respond_status(nc, err);
  free_capture_config(&cfg);
}
