/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Sync.h"

#include "NXDNDefines.h"
#include "YSFDefines.h"

#include <cstdio>
#include <cassert>
#include <cstring>

void CSync::addYSFSync(unsigned char* data)
{
	assert(data != NULL);

	::memcpy(data, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);
}

void CSync::addNXDNSync(unsigned char* data)
{
	assert(data != NULL);

	for (unsigned int i = 0U; i < NXDN_FSW_BYTES_LENGTH; i++)
		data[i] = (data[i] & ~NXDN_FSW_BYTES_MASK[i]) | NXDN_FSW_BYTES[i];
}
