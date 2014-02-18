//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011 Leo Meyer (leo@leomeyer.de)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Driver for pressure sensor BMP085

/*
 * Based on:     BMP085 Test Code
	April 7, 2010
	by: Jim Lindblom
* and: AVR TWI Example code
 */

uint8_t bmp085_init(void);

uint8_t bmp085_present(void);

uint8_t bmp085ReadShort(uint8_t reg, int16_t *result);

uint8_t bmp085ReadTemp(int32_t *temp);

uint8_t bmp085ReadPressure(int32_t *pressure);

uint8_t bmp085GetRealValues(int32_t *temperature, int32_t *pressure);
