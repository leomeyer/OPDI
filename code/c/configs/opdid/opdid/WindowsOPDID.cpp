
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <Mmsystem.h>
#include <stdio.h>
#pragma comment(lib, "Winmm.lib")

#include "opdi_platformtypes.h"
#include "opdi_config.h"
#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_message.h"
#include "opdi_slave_protocol.h"

#include "WindowsOPDID.h"

// global connection mode (TCP or COM)
#define MODE_TCP 1
#define MODE_COM 2

static int connection_mode = 0;
static char first_com_byte = 0;

static unsigned long last_activity = 0;

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


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

	while (1) {
		// call work function
		if (canSend) {
			uint8_t waitResult = Opdi->waiting(canSend);
			if (waitResult != OPDI_STATUS_OK)
				return waitResult;
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
				Opdi->logNormal("Network error: " + WSAGetLastError());
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

WindowsOPDID::WindowsOPDID(void)
{
}

WindowsOPDID::~WindowsOPDID(void)
{
}

uint32_t WindowsOPDID::getTimeMs(void) {
	return GetTickCount();
}

void WindowsOPDID::print(const char *text) {
	// text is treated as UTF8. Convert to wide character
	std::wcout << utf8_decode(std::string(text));
}

void WindowsOPDID::println(const char *text) {
	// text is treated as UTF8. Convert to wide character
	std::wcout << utf8_decode(std::string(text)) << std::endl;
}

void WindowsOPDID::printe(const char *text) {
	// text is treated as UTF8. Convert to wide character
	std::wcerr << utf8_decode(std::string(text));
}

void WindowsOPDID::printlne(const char *text) {
	// text is treated as UTF8. Convert to wide character
	std::wcerr << utf8_decode(std::string(text)) << std::endl;
}

/** This method handles an incoming TCP connection. It blocks until the connection is closed.
*/
int WindowsOPDID::HandleTCPConnection(int *csock) {
	opdi_Message message;
	uint8_t result;

	connection_mode = MODE_TCP;

	// info value is the socket handle
	opdi_message_setup(&io_receive, &io_send, (void *)csock);

	result = opdi_get_message(&message, OPDI_CANNOT_SEND);
	if (result != 0) 
		return result;

	last_activity = GetTickCount();

	// initiate handshake
	result = opdi_slave_start(&message, NULL, &protocol_callback);

    return result;
}

int WindowsOPDID::setupTCP(std::string interface_, int port) {

	int err = 0;

    // Initialize socket support WINDOWS ONLY!
    unsigned short wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
            HIBYTE(wsaData.wVersion) != 2)) {
				throw Poco::ApplicationException("Could not find useable sock dll", WSAGetLastError());
    }

    // Initialize sockets and set any options
    int hsock;
    int *p_int ;
    hsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hsock == -1) {
        throw Poco::ApplicationException("Error initializing socket", WSAGetLastError());
    }

	// make socket non-blocking
	u_long iMode = 1;
	ioctlsocket(hsock, FIONBIO, &iMode);

    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;
    if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1)||
        (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)) {
        free(p_int);
        throw Poco::ApplicationException("Error setting socket options", WSAGetLastError());
    }
    free(p_int);

    // Bind and listen
    struct sockaddr_in my_addr;

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY;	// TODO
    
    if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        throw Poco::ApplicationException("Error binding to socket, make sure nothing else is listening on this port", WSAGetLastError());
    }
    if (listen(hsock, 10) == -1) {
        throw Poco::ApplicationException("Error listening on socket", WSAGetLastError());
    }
    
	// wait for connections

    sockaddr_in sadr;
    int addr_size = sizeof(SOCKADDR);
    
    while (true) {
		if (Opdi->logVerbosity != QUIET)
			this->log(std::string("Listening for a connection on TCP port ") + this->to_string(port));

		while (true) {
			int csock = accept(hsock, (SOCKADDR *)&sadr, &addr_size);

			// error condition?
			if (csock == INVALID_SOCKET) {
				int lastError = WSAGetLastError();
				if (lastError == WSAEWOULDBLOCK) {
					// not yet connected; process housekeeping about every millisecond
					uint8_t waitResult = this->waiting(false);
					if (waitResult != OPDI_STATUS_OK)
						return waitResult;
					Sleep(1);
				} else 
					this->log(std::string("Error accepting connection: ") + this->to_string(lastError));
			} else {
				if (Opdi->logVerbosity != QUIET)
					this->log((std::string("Connection attempt from ") + std::string(inet_ntoa(sadr.sin_addr))).c_str());

				err = HandleTCPConnection(&csock);
			
				// shutdown requested?
				if (this->shutdownRequested)
					return OPDI_SHUTDOWN;

				if (Opdi->logVerbosity != QUIET)
					this->log(std::string("Result: ") + this->getOPDIResult(err));

				break;
			}
		}
	}

	return 0;
}

IOPDIDPlugin *WindowsOPDID::getPlugin(std::string driver) {

	this->warnIfPluginMoreRecent(driver);

	// attempt to load the specified DLL
	HINSTANCE dllHandle = LoadLibraryW(utf8_decode(driver).c_str());

	if (!dllHandle) {
		throw Poco::FileException("Could not load the plugin DLL", driver);
	}

	GetOPDIDPluginInstance_t getPluginInstance = reinterpret_cast<GetOPDIDPluginInstance_t>(::GetProcAddress(dllHandle, "GetOPDIDPluginInstance"));

	if (!getPluginInstance) {
		::FreeLibrary(dllHandle);
		throw Poco::ApplicationException("Invalid plugin driver DLL (could not locate function 'GetOPDIDPluginInstance')", driver);
	}

	// call the DLL function to get the plugin instance
	return getPluginInstance(this->majorVersion, this->minorVersion, this->patchVersion);
}
