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
    
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "opdi_constants.h"
#include "slave.h"

// Listen to incoming TCP requests. Supply the port you want the server to listen on
int listen_tcp(int host_port) {

	// adapted from: http://www.tidytutorials.com/2009/12/c-winsock-example-using-client-server.html

	int err = 0;

    // Initialize socket support WINDOWS ONLY!
    unsigned short wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
            HIBYTE(wsaData.wVersion) != 2)) {
        fprintf(stderr, "Could not find useable sock dll %d\n", WSAGetLastError());
        goto FINISH;
    }

    // Initialize sockets and set any options
    int hsock;
    int *p_int ;
    hsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hsock == -1) {
        printf("Error initializing socket %d\n", WSAGetLastError());
        goto FINISH;
    }
    
    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;
    if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1)||
        (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)) {
        printf("Error setting options %d\n", WSAGetLastError());
        free(p_int);
        goto FINISH;
    }
    free(p_int);

    // Bind and listen
    struct sockaddr_in my_addr;

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(host_port);
    
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY ;
    
    if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n", WSAGetLastError());
        goto FINISH;
    }
    if (listen(hsock, 10) == -1) {
        fprintf(stderr, "Error listening %d\n", WSAGetLastError());
        goto FINISH;
    }
    
	// wait for connections

    int *csock;
    sockaddr_in sadr;
    int addr_size = sizeof(SOCKADDR);
    
    while (true) {
        printf("Listening for a connection on port %d\n", host_port);
        csock = (int *)malloc(sizeof(int));
        
        if ((*csock = accept(hsock, (SOCKADDR *)&sadr, &addr_size)) != INVALID_SOCKET) {
            printf("Connection attempt from %s\n", inet_ntoa(sadr.sin_addr));

            err = HandleTCPConnection(csock);
			
			fprintf(stderr, "Result: %d\n", err);
        }
        else{
            fprintf(stderr, "Error accepting %d\n",WSAGetLastError());
        }
	}

FINISH:
	return err;
}


int listen_com(LPCTSTR lpPortName, DWORD dwBaudRate, BYTE bStopBits, BYTE bParity, BYTE bByteSize, DWORD dwTimeout) {

	// adapted from: http://www.codeproject.com/Articles/3061/Creating-a-Serial-communication-on-Win32

	int err = 0;
	DCB config;
	HANDLE hndPort = INVALID_HANDLE_VALUE;
	COMMTIMEOUTS comTimeOut;                   

	// open port
	hndPort = CreateFile(lpPortName,  
		GENERIC_READ | GENERIC_WRITE,     
		0,                                
		NULL,                             
		OPEN_EXISTING,                  
		0,                                
		NULL); 
	if (hndPort == INVALID_HANDLE_VALUE) {
		err = OPDI_DEVICE_ERROR;
        fprintf(stderr, "Error opening COM port: %d\n", GetLastError());
        goto FINISH;
	}

	// get configuration
	if (GetCommState(hndPort, &config) == 0) {
		err = OPDI_DEVICE_ERROR;
        fprintf(stderr, "Error getting COM port configuration: %d\n", GetLastError());
		goto FINISH;
	}

	// adjust configuration
	if (dwBaudRate != -1)
		config.BaudRate = dwBaudRate;
	if (bStopBits != -1)
		config.StopBits = bStopBits;
	if (bParity != -1)
		config.Parity = bParity;
	if (bByteSize != -1)
		config.ByteSize = bByteSize;

	// set configuration
	if (SetCommState(hndPort,&config) == 0) {
		err = OPDI_DEVICE_ERROR;
        fprintf(stderr, "Error setting COM port configuration: %d\n", GetLastError());
		goto FINISH;
	}

	// timeout specified?
	if (dwTimeout != -1) {
		comTimeOut.ReadIntervalTimeout = dwTimeout;
		comTimeOut.ReadTotalTimeoutMultiplier = 1;
		comTimeOut.ReadTotalTimeoutConstant = 0;
		comTimeOut.WriteTotalTimeoutMultiplier = 1;
		comTimeOut.WriteTotalTimeoutConstant = dwTimeout;
		// set the time-out parameters
		if (SetCommTimeouts(hndPort, &comTimeOut) == 0) {
			err = OPDI_DEVICE_ERROR;
			fprintf(stderr, "Unable to set timeout configuration: %d\n", GetLastError());
			goto FINISH;
		}
	}

	// wait for connections

	char inputData;
	DWORD bytesRead;

    while (true) {
        printf("Listening for a connection on COM port %s\n", lpPortName);
        
		while (true) {
			// try to read a byte
			if (ReadFile(hndPort, &inputData, 1, &bytesRead, NULL) != 0) {
				if (bytesRead > 0) {
					printf("Connection attempt on COM port\n");

					err = HandleCOMConnection(inputData, hndPort);
			
					fprintf(stderr, "Result: %d\n", err);
				}
			}
			else {
				err = GetLastError();
				// timeouts are expected here
				if (err != ERROR_TIMEOUT) {
					fprintf(stderr, "Error accepting data on COM port: %d\n", err);
					err = OPDI_DEVICE_ERROR;
					goto FINISH;
				}
			}
		}
	}

FINISH:

	// cleanup
	if (hndPort != INVALID_HANDLE_VALUE) {
		// ignore errors
		CloseHandle(hndPort);
	}

	return err;
}