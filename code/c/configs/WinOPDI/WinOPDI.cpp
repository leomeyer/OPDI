//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-2014 Leo Meyer (leo@leomeyer.de)
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
    
// WinOPDI.cpp : Defines the entry point for the console application
// 

#include <stdlib.h>

#include "stdafx.h"
#include "master.h"
#include "slave.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int code = 0;
	int interactive = 0;
	int tcp_port = 13110;
	int com_port = 0;

	printf("WinOPDI test program. Arguments: [-i] [-tcp <port>] [-com <port>]\n");
	printf("-i starts the interactive master.\n");

	for (int i = 1; i < argc; i++) {
        if (_tcscmp(argv[i], _T("-i")) == 0) {
			interactive = 1;
		} else
        if (i < argc - 1) {
            if (_tcscmp(argv[i], _T("-tcp")) == 0) {
				// parse tcp port number
				tcp_port = _wtoi(argv[++i]);
				if ((tcp_port < 1) || (tcp_port > 65535)) {
					printf("Invalid TCP port number: %d\n", tcp_port);
					exit(1);
				}
            } else if (_tcscmp(argv[i], _T("-com")) == 0) {
				// parse com port number
				com_port = _wtoi(argv[++i]);
				if ((com_port < 1) || (com_port > 32)) {
					printf("Invalid COM port number: %d\n", com_port);
					exit(1);
				}
            } else {
				printf("Unrecognized argument: %s\n", argv[i]);
				exit(1);
			}
        }
		else {
			printf("Invalid syntax: missing argument\n");
			exit(1);
		}
    }

	// master?
	if (interactive) {
		code = start_master();
	} else {
		// slave
		if (com_port > 0) {
			LPCTSTR comPort = (LPCTSTR)malloc(6);
			wsprintf((LPTSTR)comPort, _T("COM%d"), com_port);

			code = listen_com(comPort, -1, -1, -1, -1, 1000);
		}
		else {
			code = listen_tcp(tcp_port);
		}
	}
	printf("Exit code: %d", code);
	exit(code);
}

