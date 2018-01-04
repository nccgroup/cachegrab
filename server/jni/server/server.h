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

#ifndef SERVER_H__
#define SERVER_H__

#include <mongoose.h>
#include "cachegrab.h"

#define PORT "8000"

#define POST mg_mk_str("POST")
#define GET  mg_mk_str("GET")

#define HTTP_OK(nc) mg_printf((nc), "HTTP/1.1 200 OK\r\n\r\n")
#define HTTP_BAD(nc) mg_printf((nc), "HTTP/1.1 400 Bad Request\r\n\r\n")
#define HTTP_NOTFOUND(nc) mg_printf((nc), "HTTP/1.1 404 File Not Found\r\n\r\n")
#define HTTP_ERR(nc) mg_printf((nc), "HTTP/1.1 500 Server Error\r\n\r\n")
#define HTTP_DONE(nc) (nc)->flags |= MG_F_SEND_AND_CLOSE

void print_status (struct mg_connection *nc, enum CGState s);
void respond_status (struct mg_connection *nc, enum CGState s);
void print_buf (struct mg_connection *nc, uint8_t* buf, size_t len);

#endif
