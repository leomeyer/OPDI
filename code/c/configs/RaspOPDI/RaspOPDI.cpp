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
    
// RaspOPDI.cpp : Defines the entry point for the console application
// 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "server.h"

int main(int argc, char* argv[])
{
	int code = 0;
	int tcp_port = 13110;
	char *comPort = NULL;

	printf("RaspOPDI server. Arguments: [-tcp <port>] [-serial <device>]\n");

	for (int i = 1; i < argc; i++) {
            if (i < argc - 1) {
                if (strcmp(argv[i], "-tcp") == 0) {
					// parse tcp port number
					tcp_port = atoi(argv[++i]);
					if ((tcp_port < 1) || (tcp_port > 65535)) {
						printf("Invalid TCP port number: %d\n", tcp_port);
						exit(1);
					}
                } else if (strcmp(argv[i], "-serial") == 0) {
			comPort = argv[++i];
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

	if (comPort != NULL) {
		code = listen_com(comPort, -1, -1, -1, -1, 1000);
	}
	else {
		code = listen_tcp(tcp_port);
	}

	printf("Exit code: %d\n", code);
	exit(code);
}

