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

#include <mongoose.h>
#include <stdio.h>

#include "scope.h"
#include "server_capture.h"
#include "server_probe.h"
#include "server_scope.h"

static struct mg_mgr mgr;
static struct mg_connection *nc;

void print_status (struct mg_connection *nc, enum CGState s) {
  mg_printf(nc, "\"status\": \"");

  switch(s) {
  case CG_OK:
    mg_printf(nc, "Success");
    break;
  case CG_BAD_ARG:
    mg_printf(nc, "Invalid Argument");
    break;
  case CG_PERM:
    mg_printf(nc, "Incorrect Permissions");
    break;
  case CG_SCOPE_ALREADY_CONNECTED:
    mg_printf(nc, "Scope has already been connected");
    break;
  case CG_SCOPE_NOT_CONNECTED:
    mg_printf(nc, "Scope is not connected");
    break;
  case CG_PROBE_ALREADY_CONNECTED:
    mg_printf(nc, "Probe has already been connected");
    break;
  case CG_PROBE_NOT_CONNECTED:
    mg_printf(nc, "Probe is not connected");
    break;
  case CG_NO_MEM:
    mg_printf(nc, "Memory allocation failure");
    break;
  case CG_BAD_CMD:
    mg_printf(nc, "Command Not Recognized");
    break;
  case CG_INTERNAL_ERR:
    mg_printf(nc, "Internal Error");
    break;
  case CG_CAPTURE_ERR:
    mg_printf(nc, "Failure to setup capture");
    break;
  default:
    mg_printf(nc, "Unknown");
  }
  mg_printf(nc, "\"");
}

void respond_status (struct mg_connection *nc, enum CGState s) {
  if (s == CG_OK)
    HTTP_OK(nc);
  else if (s == CG_INTERNAL_ERR)
    HTTP_ERR(nc);
  else
    HTTP_BAD(nc);

  mg_printf(nc, "{");
  print_status(nc, s);
  mg_printf(nc, "}");

  HTTP_DONE(nc);
}

void print_buf (struct mg_connection *nc, uint8_t* buf, size_t len) {
  // Print a buffer as hexadecimal to the connection
  for (size_t i = 0; i < len; i++) {
    mg_printf(nc, "%02x", buf[i]);
  }
}


static void ev_handler(struct mg_connection *nc, int ev, void *data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    HTTP_BAD(nc);
    HTTP_DONE(nc);
  }
}

int server_init (void) {
  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, PORT , ev_handler);
  if (nc == NULL) {
    fprintf(stderr, "Could not bind to port.\n");
    return -1;
  }
  mg_register_http_endpoint(nc, "/system", handle_system);
  mg_register_http_endpoint(nc, "/configuration", handle_status);
  mg_register_http_endpoint(nc, "/connect", handle_connect);
  mg_register_http_endpoint(nc, "/disconnect", handle_disconnect);

  mg_register_http_endpoint(nc, "/l1d/configuration", handle_l1d_config);
  mg_register_http_endpoint(nc, "/l1d/connect", handle_l1d_connect);
  mg_register_http_endpoint(nc, "/l1d/disconnect", handle_l1d_disconnect);
  mg_register_http_endpoint(nc, "/capture/l1d.png", handle_l1d_data);
  
  mg_register_http_endpoint(nc, "/l1i/configuration", handle_l1i_config);
  mg_register_http_endpoint(nc, "/l1i/connect", handle_l1i_connect);
  mg_register_http_endpoint(nc, "/l1i/disconnect", handle_l1i_disconnect);
  mg_register_http_endpoint(nc, "/capture/l1i.png", handle_l1i_data);
  
  mg_register_http_endpoint(nc, "/btb/configuration", handle_btb_config);
  mg_register_http_endpoint(nc, "/btb/connect", handle_btb_connect);
  mg_register_http_endpoint(nc, "/btb/disconnect", handle_btb_disconnect);
  mg_register_http_endpoint(nc, "/capture/btb.png", handle_btb_data);

  mg_register_http_endpoint(nc, "/capture/start", handle_capture);

  mg_set_protocol_http_websocket(nc);
  return 0;
}

void server_term (void) {
  mg_mgr_free(&mgr);
}

void server_run (void) {
  while (1) {
    mg_mgr_poll(&mgr, 1000);
  }
}

int main (int argc, char** argv) {
  if (scope_init() < 0) {
    fprintf(stderr, "Failed to initialize the scope.\n");
    return -1;
  }

  if (server_init() < 0) {
    fprintf(stderr, "Failed to initialize the server.\n");
    return -1;
  }

  daemon(0, 0);
  server_run();

  server_term();
  scope_term();

  return 0;
}
