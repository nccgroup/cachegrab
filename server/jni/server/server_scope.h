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

#ifndef SERVER_SCOPE_H__
#define SERVER_SCOPE_H__

#include <mongoose.h>

void handle_system (struct mg_connection *nc, int ev, void *data);
void handle_status (struct mg_connection *nc, int ev, void *data);
void handle_connect (struct mg_connection *nc, int ev, void *data);
void handle_disconnect (struct mg_connection *nc, int ev, void *data);

#endif
