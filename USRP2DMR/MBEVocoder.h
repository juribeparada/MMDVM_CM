/* 
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2020 by Doug McLain AD8DP
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

#ifndef INCLUDED_AMBE_ENCODER_H
#define INCLUDED_AMBE_ENCODER_H

#include "mbeenc.h"
#include <stdint.h>


class MBEVocoder {
public:
	void decode_2450(int16_t *, uint8_t *);
	void encode_2450(int16_t *, uint8_t *);
	void encode_dmr(int16_t *, uint8_t *);
	MBEVocoder(void);

private:
	MBEEncoder *m_mbeenc;
};

#endif /* INCLUDED_AMBE_ENCODER_H */
