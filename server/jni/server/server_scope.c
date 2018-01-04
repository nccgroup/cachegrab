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

#include "server.h"
#include "server_scope.h"

#include "scope.h"
#include "server_probe.h"

void handle_system (struct mg_connection *nc, int ev, void *data) {
  struct http_message *msg = data;
  
  if (0 != mg_strcmp(GET, msg->method)) {
    respond_status(nc, CG_BAD_CMD);
    return;
  }

  HTTP_OK(nc);
  mg_printf(nc, "{");
  print_status(nc, CG_OK);
  mg_printf(nc, ", ");
  mg_printf(nc, "\"num_cores\": %ld", sysconf( _SC_NPROCESSORS_ONLN ));
  mg_printf(nc, "}");
  HTTP_DONE(nc);
}

void handle_status (struct mg_connection *nc, int ev, void *data) {
  struct http_message *msg = data;
  enum CGState ret;
  struct scope *s;
  
  if (0 != mg_strcmp(GET, msg->method)) {
    respond_status(nc, CG_BAD_CMD);
    return;
  }

  ret = scope_get_configuration(&s);

  if (ret != CG_OK) {
    respond_status(nc, ret);
    return;
  }

  HTTP_OK(nc);
  mg_printf(nc, "{");
  print_status(nc, CG_OK);
  mg_printf(nc, ", \"scope\": {");
  
  if (s->connected) {
    mg_printf(nc, "\"target_cpu\": %d", s->target_cpu);
    mg_printf(nc, ", \"scope_cpu\": %d", s->scope_cpu);
    mg_printf(nc, ", \"probes\": {");

    mg_printf(nc, "\"l1d\": {");
    if (s->l1d.attached) {
      print_probe(nc, &s->l1d);
    }
    mg_printf(nc, "}, \"l1i\": {");
    if (s->l1i.attached) {
      print_probe(nc, &s->l1i);
    }
    mg_printf(nc, "}, \"btb\": {");
    if (s->btb.attached) {
      print_probe(nc, &s->btb);
    }
    mg_printf(nc, "}");
    
    mg_printf(nc, "}");
  }
  mg_printf(nc, "}}");
  HTTP_DONE(nc);
}

void handle_connect (struct mg_connection *nc, int ev, void *data) {
  struct http_message *msg = data;
  struct mg_str *body = &msg->body;
  enum CGState err = CG_BAD_ARG;
  char tcpu_str[8];
  char scpu_str[8];
  int tcpu, scpu;
  
  if (0 != mg_strcmp(POST, msg->method))
    goto done;

  if (mg_get_http_var(body, "target_cpu", tcpu_str, sizeof(tcpu_str)) <= 0 ||
      1 != sscanf(tcpu_str, "%d", &tcpu) ||
      tcpu < 0)
    goto done;

  if (mg_get_http_var(body, "scope_cpu", scpu_str, sizeof(scpu_str)) <= 0 ||
      1 != sscanf(scpu_str, "%d", &scpu) ||
      scpu < 0 ||
      scpu == tcpu)
    goto done;

  err = scope_connect(tcpu, scpu);
  if (err != CG_OK)
    goto done;

  err = CG_OK;
 done:
  respond_status(nc, err);
}

void handle_disconnect (struct mg_connection *nc, int ev, void *data) {
  struct http_message *msg = data;
  enum CGState err = CG_BAD_ARG;
  if (0 != mg_strcmp(POST, msg->method))
    goto done;

  scope_disconnect();
  err = CG_OK;
 done:
  respond_status(nc, err);
}
