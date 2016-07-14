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
    

    
#ifdef __cplusplus
extern "C" {
#endif 

/** Handles an incoming TCP connection by performing the handshake and running the message loop
*   on the specified socket.
*/
extern int HandleTCPConnection(int csock);


/** Handles an incoming serial connection by performing the handshake and running the message loop.
*/
extern int HandleSerialConnection(char firstByte, int hndPort);

#ifdef __cplusplus
}
#endif


/** Starts the server by listening to the specified TCP port. 
*/
int listen_tcp(int host_port);

/** Starts the server by listening to the specified COM port.
*/
int listen_com(char* portName, int baudRate, int stopBits, int parity, int byteSize, int tmeout);
