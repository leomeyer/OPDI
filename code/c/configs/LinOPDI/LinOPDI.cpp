//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-2015 Leo Meyer (leo@leomeyer.de)
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

// LinOPDI.cpp : Defines the entry point for the console application
//

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/param.h>

#include "opdi_platformfuncs.h"
#include "opdi_configspecs.h"
#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_message.h"
#include "opdi_protocol.h"
#include "opdi_slave_protocol.h"
#include "opdi_config.h"

#include "../test/test.h"
#include "../test/master.h"

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_SERIAL 2

static int connection_mode = 0;
static char first_com_byte = 0;

static unsigned long idle_timeout_ms = 180000;
static unsigned long last_activity = 0;

/** For TCP connections, receives a byte from the socket specified in info and places the result in byte.
*   For serial connections, reads a byte from the file handle specified in info and places the result in byte.
*   Blocks until data is available or the timeout expires.
*   If an error occurs returns an error code != 0.
*   If the connection has been gracefully closed, returns STATUS_DISCONNECTED.
*/
static uint8_t io_receive(void *info, uint8_t *byte, uint16_t timeout, uint8_t canSend) {
	char c;
	int result;
	uint64_t ticks = opdi_get_time_ms();
	long sendTicks = ticks;

	while (1) {
		// send a message every few ms if canSend
		// independent of connection mode
		if (opdi_get_time_ms() - sendTicks >= 830) {
			if (canSend) {
				sendTicks = opdi_get_time_ms();

				handle_streaming_ports();
			}
		}

		if (connection_mode == MODE_TCP) {

			int newsockfd = (long)info;

			// try to read data
			result = read(newsockfd, &c, 1);
			if (result < 0) {
				// timed out?
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// possible timeout
					// "real" timeout condition
					if (opdi_get_time_ms() - ticks >= timeout)
						return OPDI_TIMEOUT;
				}
				else {
					// other error condition
					perror("ERROR reading from socket");
					return OPDI_NETWORK_ERROR;
				}
			}
			else
			// connection closed?
			if (result == 0)
				// dirty disconnect
				return OPDI_NETWORK_ERROR;
			else
				// a byte has been received
//				printf("%i", c);
				break;
		}
		else
		if (connection_mode == MODE_SERIAL) {
			int fd = (long)info;
			char inputData;
			int bytesRead;

			// first byte of connection remembered?
			if (first_com_byte != 0) {
				c = first_com_byte;
				first_com_byte = 0;
				break;
			}

			if ((bytesRead = read(fd, &inputData, 1)) >= 0) {
				if (bytesRead == 1) {
					// a byte has been received
					c = inputData;
					break;
				}
				else {
					// ran into timeout
					// "real" timeout condition
					if (opdi_get_time_ms() - ticks >= timeout)
						return OPDI_TIMEOUT;
				}
			}
			else {
				// device error
				return OPDI_DEVICE_ERROR;
			}
		}
	}

	*byte = (uint8_t)c;

	return OPDI_STATUS_OK;
}

/** For TCP connections, sends count bytes to the socket specified in info.
*   For serial connections, writes count bytes to the file handle specified in info.
*   If an error occurs returns an error code != 0. */
static uint8_t io_send(void *info, uint8_t *bytes, uint16_t count) {
	char *c = (char *)bytes;

	if (connection_mode == MODE_TCP) {

		int newsockfd = (long)info;

		if (write(newsockfd, c, count) < 0) {
			printf("ERROR writing to socket");
			return OPDI_DEVICE_ERROR;
		}
	}
	else
	if (connection_mode == MODE_SERIAL) {
		int fd = (long)info;

		if (write(fd, c, count) != count) {
			return OPDI_DEVICE_ERROR;
		}
	}

	return OPDI_STATUS_OK;
}

void init_device() {
	// set slave name
	char nameBuf[1024];
	size_t compNameLen = 1024;

	if (opdi_config_name == NULL) {
		opdi_config_name = (const char*)malloc(OPDI_CONFIG_NAME_LENGTH);
	}

	// requires #undef UNICODE to work properly
	if (0 != gethostname(nameBuf, compNameLen)) {
		sprintf((char *)opdi_config_name, "LinOPDI (%s)", nameBuf);
	}
	else {
		opdi_config_name = "LinOPDI";
	}

	// configure ports
	configure_ports();
}

#ifdef __cplusplus
extern "C" {
#endif 

/** This method handles an incoming TCP connection. It blocks until the connection is closed.
*/
int HandleTCPConnection(int csock) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_TCP;
	init_device();

	struct timeval aTimeout;
	aTimeout.tv_sec = 0;
	aTimeout.tv_usec = 1000;		// one ms timeout

	// set timeouts on socket
	if (setsockopt (csock, SOL_SOCKET, SO_RCVTIMEO, (char *)&aTimeout, sizeof(aTimeout)) < 0) {
		printf("setsockopt failed\n");
		return OPDI_DEVICE_ERROR;
	}
	if (setsockopt (csock, SOL_SOCKET, SO_SNDTIMEO, (char *)&aTimeout, sizeof(aTimeout)) < 0) {
		printf("setsockopt failed\n");
		return OPDI_DEVICE_ERROR;
	}

	// info value is the socket handle
	result = opdi_message_setup(&io_receive, &io_send, (void *)(long)csock);
	if (result != 0) 
		return result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = opdi_get_time_ms();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

	// release the socket
	return result;
}

/** This method handles an incoming serial connection. It blocks until the connection is closed.
*/
int HandleSerialConnection(char firstByte, int fd) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_SERIAL;
	first_com_byte = firstByte;
	init_device();

	// info value is the serial port handle
	result = opdi_message_setup(&io_receive, &io_send, (void *)(long)fd);
	if (result != 0)
		return result;

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0)
		return result;

	last_activity = opdi_get_time_ms();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &my_protocol_callback);

	return result;
}

#ifdef __cplusplus
}
#endif 


uint8_t opdi_message_handled(channel_t channel, const char **parts) {
	uint8_t result;
	if (idle_timeout_ms > 0) {
		// do not time out if there are bound streaming ports
		if (channel != 0 || opdi_get_port_bind_count() > 0) {
			// reset activity time
			last_activity = opdi_get_time_ms();
		} else {
			// control channel message
			if (opdi_get_time_ms() - last_activity > idle_timeout_ms) {
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
// Listen to incoming TCP requests. Supply the port you want the server to listen on
int listen_tcp(int host_port) {
	// adapted from: http://www.linuxhowtos.org/C_C++/socket.htm

	int err = 0;

	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("ERROR opening socket\n");
		err = OPDI_DEVICE_ERROR;
		goto FINISH;
	}

	// prepare address
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(host_port);

	// bind to specified port
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding\n");
		err = OPDI_DEVICE_ERROR;
		goto FINISH;
	}

	// listen for incoming connections
	// set maximum queue size to 5
	listen(sockfd, 5);

	while (true) {
        	printf("listening for a connection on port %d\n", host_port);
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0) {
			printf("ERROR on accept\n");
			err = OPDI_DEVICE_ERROR;
			goto FINISH;
		}

		printf("Connection attempt from %s\n", inet_ntoa(cli_addr.sin_addr));

		err = HandleTCPConnection(newsockfd);

		close(newsockfd);
		fprintf(stderr, "Result: %d\n", err);
        }

FINISH:
	if (sockfd > 0)
		close(sockfd);
	return err;
}


int listen_com(char* portName, int baudRate, int stopBits, int parity, int byteSize, int timeout) {

	// adapted from: http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c

	struct termios tty;
	int err = 0;

	int fd = open(portName, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		printf("error %d opening %s: %s\n", errno, portName, strerror (errno));
		err = OPDI_DEVICE_ERROR;
		goto FINISH;
	}

        memset (&tty, 0, sizeof tty);
        if (tcgetattr(fd, &tty) != 0)
        {
                printf("error %d from tcgetattr\n", errno);
		err = OPDI_DEVICE_ERROR;
		goto FINISH;
        }

	if (baudRate != -1)
	{
		// TODO calculate call constant from baud rate
		cfsetospeed(&tty, B9600);
		cfsetispeed(&tty, B9600);
	}

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = (timeout < 1 ? 1 : 0);	// block if no timeout specified
        tty.c_cc[VTIME] = (timeout < 1 ? 0 : (timeout / 100));	// read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
	if (parity == 0)
	        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	else
	if (parity == 1) {
	        tty.c_cflag |= PARENB | PARODD;		// parity odd
	} else
	if (parity == 2) {
	        tty.c_cflag &= ~(PARODD);		// parity even
		tty.c_cflag |= PARENB;
	}

	if (stopBits == 2) {
        	tty.c_cflag |= CSTOPB;
	} else
	if (stopBits == 1) {
        	tty.c_cflag &= ~CSTOPB;
	}

	if (byteSize == 5) {
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS5;
	} else
	if (byteSize == 6) {
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS6;
	} else
	if (byteSize == 7) {
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS7;
	} else
	if (byteSize == 8) {
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;
	}

        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf("error %d from tcsetattr\n", errno);
		err = OPDI_DEVICE_ERROR;
		goto FINISH;
        }

	// wait for connections

	char inputData;
	int bytesRead;

	while (true) {
		printf("listening for a connection on serial port %s\n", portName);

		while (true) {
			// try to read a byte
			if ((bytesRead = read(fd, &inputData, 1)) >= 0) {
				if (bytesRead > 0) {
					printf("Connection attempt on serial port\n");

					err = HandleSerialConnection(inputData, fd);

					fprintf(stderr, "Result: %d\n", err);
					break;
				}
			}
			else {
				// timeouts are expected here
				// TODO
			}
		}
	}

FINISH:

	// cleanup
	if (fd > 0) {
		close(fd);
	}

	return err;
}

int main(int argc, char* argv[])
{
	int code = 0;
	int interactive = 0;
	int tcp_port = 13110;
	char *comPort = NULL;

	printf("LinOPDI server. Arguments: [-i] [-tcp <port>] [-com <port>]\n");
	printf("-i starts the interactive master.\n");

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			interactive = 1;
		} else
        	if (i < argc - 1) {
	                if (strcmp(argv[i], "-tcp") == 0) {
				// parse tcp port number
				tcp_port = atoi(argv[++i]);
				if ((tcp_port < 1) || (tcp_port > 65535)) {
					printf("Invalid TCP port number: %d\n", tcp_port);
					exit(1);
				}
        	        } else if (strcmp(argv[i], "-com") == 0) {
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

	// master?
	if (interactive) {
		code = start_master();
	} else {
		// slave
		if (comPort != NULL) {
			code = listen_com(comPort, -1, -1, -1, -1, 1000);
		}
		else {
			code = listen_tcp(tcp_port);
		}
	}

	printf("Exit code: %d\n", code);
	exit(code);
}
