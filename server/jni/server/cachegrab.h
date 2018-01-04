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

#ifndef CACHEGRAB_H__
#define CACHEGRAB_H__

enum CGState {
	CG_OK = 0, //!< Operation successful
	CG_BAD_ARG, //!< Error in arguments
	CG_PERM, //!< Permissions error in accessing memory
	CG_SCOPE_ALREADY_CONNECTED, //!< The scope already exists
	CG_SCOPE_NOT_CONNECTED, //!< The scope is not connected
	CG_PROBE_ALREADY_CONNECTED, //!< The probe already exists
	CG_PROBE_NOT_CONNECTED, //!< The probe is not connected
	CG_NO_MEM, //!< Memory allocation failure
	CG_BAD_CMD, //!< Command does not exist
	CG_INTERNAL_ERR, //!< An unknown internal error occured
	CG_CAPTURE_ERR, //!< An error occured while setting up capture
};

#endif
