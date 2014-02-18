//    This file is part of a OPDI reference implementation.
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
    
// Implements encryption functions using AES.

#include "opdi_constants.h"
#include "opdi_device.h"
#include "opdi_configspecs.h"

#include "opdi_rijndael.h"

static CRijndael *rijndael = NULL;

static CRijndael *get_rijndael() {
	if (rijndael != NULL) {
		delete rijndael;
	}
	if (strlen(opdi_encryption_key) != OPDI_ENCRYPTION_BLOCKSIZE)
		throw runtime_error("AES encryption key length does not match the block size");

	rijndael = new CRijndael();
	rijndael->MakeKey(opdi_encryption_key, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", OPDI_ENCRYPTION_BLOCKSIZE, OPDI_ENCRYPTION_BLOCKSIZE);
	return rijndael;
}


uint8_t opdi_encrypt_block(uint8_t *dest, const uint8_t *src) {
	try
	{
		CRijndael *oRijndael = get_rijndael();
		oRijndael->EncryptBlock((const char*)src, (char*)dest);
	} catch (exception) {
		return OPDI_ENCRYPTION_ERROR;
	}

	return OPDI_STATUS_OK;
}


uint8_t opdi_decrypt_block(uint8_t *dest, const uint8_t *src) {
	try
	{
		CRijndael *oRijndael = get_rijndael();
		oRijndael->DecryptBlock((const char*)src, (char*)dest);
	} catch (exception) {
		return OPDI_ENCRYPTION_ERROR;
	}

	return OPDI_STATUS_OK;
}
