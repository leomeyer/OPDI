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

#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <WinBase.h>
#include <Mmsystem.h>
#include <stdio.h>
#pragma comment(lib, "Winmm.lib")

#include "opdi_platformtypes.h"
#include "opdi_config.h"
#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_message.h"
#include "opdi_slave_protocol.h"

#include "stdafx.h"

#include "test.h"
#include "master.h"

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_COM 2

static int connection_mode = 0;
static char first_com_byte = 0;

static unsigned long idle_timeout_ms = 180000;
static unsigned long last_activity = 0;

/** For TCP connections, receives a byte from the socket specified in info and places the result in byte.
*   For COM connections, reads a byte from the file handle specified in info and places the result in byte.
*   Blocks until data is available or the timeout expires. 
*   If an error occurs returns an error code != 0. 
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {
	char c;
	int result;
	long ticks = GetTickCount();
	long sendTicks = ticks;

	while (1) {
		// send a message every few ms if canSend
		// independent of connection mode
		if (GetTickCount() - sendTicks >= 999) {
			if (canSend) {
				sendTicks = GetTickCount();

				handle_streaming_ports();
			}
		}

		if (connection_mode == MODE_TCP) {
			int *csock = (int *)info;
			fd_set sockset;
			TIMEVAL aTimeout;
			// wait until data arrives or a timeout occurs
			aTimeout.tv_sec = 0;
			aTimeout.tv_usec = 1000;		// one ms timeout

			// try to receive a byte within the timeout
			FD_ZERO(&sockset);
			FD_SET(*csock, &sockset);
			if ((result = select(0, &sockset, NULL, NULL, &aTimeout)) == SOCKET_ERROR) {
				printf("Network error: %d\n", WSAGetLastError());
				return OPDI_NETWORK_ERROR;
			}
			if (result == 0) {
				// ran into timeout
				// "real" timeout condition
				if (GetTickCount() - ticks >= timeout)
					return OPDI_TIMEOUT;
			} else {
				// socket contains data
				c = '\0';
				if ((result = recv(*csock, &c, 1, 0)) == SOCKET_ERROR) {
					return OPDI_NETWORK_ERROR;
				}
				// connection closed?
				if (result == 0)
					// dirty disconnect
					return OPDI_NETWORK_ERROR;

				// a byte has been received
				break;
			}
		}
		else
		if (connection_mode == MODE_COM) {
			HANDLE hndPort = (HANDLE)info;
			char inputData;
			DWORD bytesRead;
			int err;

			// first byte of connection remembered?
			if (first_com_byte != 0) {
				c = first_com_byte;
				first_com_byte = 0;
				break;
			}

			if (ReadFile(hndPort, &inputData, 1, &bytesRead, NULL) != 0) {
				if (bytesRead == 1) {
					// a byte has been received
					c = inputData;
					break;
				}
				else {
					// ran into timeout
					// "real" timeout condition
					if (GetTickCount() - ticks >= timeout)
						return OPDI_TIMEOUT;
				}
			}
			else {
				err = GetLastError();
				// timeouts are expected here
				if (err != ERROR_TIMEOUT) {
					return OPDI_DEVICE_ERROR;
				}
			}
		}
	}

	*byte = (uint8_t)c;

	return OPDI_STATUS_OK;
}

/** For TCP connections, sends count bytes to the socket specified in info.
*   For COM connections, writes count bytes to the file handle specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	char *c = (char *)bytes;

	if (connection_mode == MODE_TCP) {
		int *csock = (int *)info;

		if (send(*csock, c, count, 0) == SOCKET_ERROR) {
			return OPDI_DEVICE_ERROR;
		}
	}
	else
	if (connection_mode == MODE_COM) {
		HANDLE hndPort = (HANDLE)info;
		DWORD length;

		if (WriteFile(hndPort, c, count, &length, NULL) == 0) {
			return OPDI_DEVICE_ERROR;
		}
	}

	return OPDI_STATUS_OK;
}

void init_device() {
	// set slave name
	TCHAR compName[1024];
	DWORD dwCompNameLen = 1024;
	TCHAR nameBuf[1024];

	// UTF-8?
	if (strcmp(opdi_encoding, OPDI_ENCODING_UTF8) == 0) {

		// get the computer name (WSTRING)
		if (0 != GetComputerName(compName, &dwCompNameLen)) {

			// print the formatted name into the buffer
			swprintf(nameBuf, dwCompNameLen, L"WinOPDI (%s)", compName);

			opdi_config_name = (char*)malloc(OPDI_CONFIG_NAME_LENGTH);

			// convert to multibyte character set (UTF-8)
			WideCharToMultiByte(CP_UTF8, 0, &nameBuf[0], -1, (LPSTR)opdi_config_name, OPDI_CONFIG_NAME_LENGTH, NULL, NULL);
		}
	} else {
		// single byte character set
	
		// TODO convert from wide char
	}

	// fallback to generic name
	if (opdi_config_name == NULL) {
		opdi_config_name = "WinOPDI";
	}

	// configure ports
	configure_ports();

	opdi_slave_init();
}


uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	uint8_t result;
	if (idle_timeout_ms > 0) {
		// do not time out if there are bound streaming ports
		if (channel != 0 || opdi_get_port_bind_count() > 0) {
			// reset activity time
			last_activity = GetTickCount();
		} else {
			// control channel message
			if (GetTickCount() - last_activity > idle_timeout_ms) {
				result = opdi_send_debug("Session timeout!");
				if (result != OPDI_STATUS_OK)
					return result;
				return opdi_disconnect();
			} else {
				// send a test debug message
				// opdi_send_debug("Session timeout not yet reached");
			}
		}
	}

	return OPDI_STATUS_OK;
}

/** This method handles an incoming TCP connection. It blocks until the connection is closed.
*/
int HandleTCPConnection(int *csock) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_TCP;
	init_device();

	// info value is the socket handle
	opdi_message_setup(&io_receive, &io_send, (void *)csock);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

	// release the socket
    free(csock);
    return result;
}

/** This method handles an incoming COM connection. It blocks until the connection is closed.
*/
int HandleCOMConnection(char firstByte, HANDLE hndPort) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_COM;
	first_com_byte = firstByte;
	init_device();

	// info value is the serial port handle
	opdi_message_setup(&io_receive, &io_send, (void *)hndPort);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

    return result;
}

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
    
    if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) != 0) {
        fprintf(stderr,"Error binding to socket, make sure nothing else is listening on port %d\n", WSAGetLastError());
        goto FINISH;
    }
    if (listen(hsock, 10) != 0) {
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

// Listen to incoming requests on a serial port.
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

// main function
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

