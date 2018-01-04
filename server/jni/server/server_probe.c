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

#include "server_probe.h"

#include "driver.h"
#include "scope.h"
#include "server.h"

bool get_cache_shape (struct cache_shape *s, struct mg_str *ps) {
  char nset_s[10];
  char nway_s[10];
  char line_s[10];

  unsigned int nset, nway, line;

  if (mg_get_http_var(ps, "num_sets", nset_s, sizeof(nset_s)) <= 0)
    return false;
  if (mg_get_http_var(ps, "associativity", nway_s, sizeof(nway_s)) <= 0)
    return false;
  if (mg_get_http_var(ps, "line_size", line_s, sizeof(line_s)) <= 0)
    return false;

  if (1 != sscanf(nset_s, "%u", &nset))
    return false;
  if (1 != sscanf(nway_s, "%u", &nway))
    return false;
  if (1 != sscanf(line_s, "%u", &line))
    return false;

  s->num_sets = nset;
  s->associativity = nway;
  s->line_size = line;
  return true;
}

void print_probe (struct mg_connection *nc, struct probe* p) {
  if (p && p->attached) {
    mg_printf(nc, "\"cache_shape\": {");
    mg_printf(nc, "\"num_sets\": %u, ", p->shape.num_sets);
    mg_printf(nc, "\"associativity\": %u, ", p->shape.associativity);
    mg_printf(nc, "\"line_size\": %u", p->shape.line_size);
    mg_printf(nc, "}, \"config\": {");
    mg_printf(nc, "\"set_start\": %u, ", p->cfg.set_start);
    mg_printf(nc, "\"set_end\": %u", p->cfg.set_end);
    mg_printf(nc, "}");
  }
}

void handle_probe_config (struct mg_connection *nc, void *data, enum probe_type t) {
  struct http_message *msg = data;
  struct mg_str *body = &msg->body;
  struct probe* p;
  enum CGState err = CG_BAD_ARG;
  
  if (0 == mg_strcmp(GET, msg->method)) {
    err = scope_get_probe_configuration(t, &p);
    if (err == CG_OK || err == CG_PROBE_ALREADY_CONNECTED) {
      HTTP_OK(nc);
      mg_printf(nc, "{");
      print_status(nc, CG_OK);
      mg_printf(nc, ", \"probe\": {");
      print_probe(nc, p);
      mg_printf(nc, "}}");
      HTTP_DONE(nc);
    } else {
      respond_status(nc, err);
    }
  } else if (0 == mg_strcmp(POST, msg->method)) {
    char start_s[10];
    char end_s[10];
    unsigned int start, end;

    if (mg_get_http_var(body, "set_start", start_s, sizeof(start_s)) > 0 &&
	mg_get_http_var(body,   "set_end",   end_s, sizeof(  end_s)) > 0 &&
	1 == sscanf(start_s, "%u", &start) &&
	1 == sscanf(  end_s, "%u",   &end)) {
      scope_set_probe_configuration(t, start, end);
      err = CG_OK;
    }
    respond_status(nc, err);
  } else {
    respond_status(nc, err);
  }
}

void handle_probe_connect (struct mg_connection *nc, void *data, enum probe_type t) {
  struct http_message *msg = data;
  enum CGState err = CG_BAD_ARG;
  struct cache_shape s;
  
  if (0 != mg_strcmp(POST, msg->method))
    goto done;

  if (!get_cache_shape(&s, &msg->body))
    goto done;

  err = scope_attach_probe(t, &s);
 done:
  respond_status(nc, err);
}

void handle_probe_disconnect (struct mg_connection *nc, void *data, enum probe_type type) {
  struct http_message *msg = data;
  enum CGState err = CG_BAD_ARG;
  if (0 != mg_strcmp(POST, msg->method))
    goto done;

  // Disconnect
  scope_detach_probe(type);
  err = CG_OK;
  
 done:
  respond_status(nc, err);
}

void handle_probe_data (struct mg_connection *nc, void *data, enum probe_type t) {
  struct http_message *msg = data;
  void* buf;
  size_t len;

  if (0 == mg_strcmp(GET, msg->method) &&
      CG_OK == scope_get_probe_data(t, &buf, &len) &&
      buf != NULL) {
    HTTP_OK(nc);
    mg_send(nc, buf, len);
    HTTP_DONE(nc);
  } else {
    HTTP_NOTFOUND(nc);
    HTTP_DONE(nc);
  }
}

void handle_l1d_config (struct mg_connection *nc, int ev, void *data) {
  handle_probe_config(nc, data, PROBE_TYPE_L1D);
}
void handle_l1d_connect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_connect(nc, data, PROBE_TYPE_L1D);
}
void handle_l1d_disconnect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_disconnect(nc, data, PROBE_TYPE_L1D);
}
void handle_l1d_data (struct mg_connection *nc, int ev, void *data) {
  handle_probe_data(nc, data, PROBE_TYPE_L1D);
}

void handle_l1i_config (struct mg_connection *nc, int ev, void *data) {
  handle_probe_config(nc, data, PROBE_TYPE_L1I);
}
void handle_l1i_connect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_connect(nc, data, PROBE_TYPE_L1I);
}
void handle_l1i_disconnect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_disconnect(nc, data, PROBE_TYPE_L1I);
}
void handle_l1i_data (struct mg_connection *nc, int ev, void *data) {
  handle_probe_data(nc, data, PROBE_TYPE_L1I);
}

void handle_btb_config (struct mg_connection *nc, int ev, void *data) {
  handle_probe_config(nc, data, PROBE_TYPE_BTB);
}
void handle_btb_connect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_connect(nc, data, PROBE_TYPE_BTB);
}
void handle_btb_disconnect (struct mg_connection *nc, int ev, void *data) {
  handle_probe_disconnect(nc, data, PROBE_TYPE_BTB);
}
void handle_btb_data (struct mg_connection *nc, int ev, void *data) {
  handle_probe_data(nc, data, PROBE_TYPE_BTB);
}
