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

/** Defines the OPDI classes that wrap the functions of the OPDI C implementation.
 */

#ifndef __OPDI_H__
#define __OPDI_H__

#include "OPDI_Ports.h"

#include "opdi_config.h"
#include "opdi_port.h"


//////////////////////////////////////////////////////////////////////////////////////////
// Main class for OPDI functionality
//////////////////////////////////////////////////////////////////////////////////////////

// All methods should be virtual (for the OPDID plugins to work)
class OPDI {

protected:
	// indicates that the OPDI system should shutdown
	bool shutdownRequested;
	// internal flag for the methods that possibly send messages
	uint8_t canSend;

	// list pointers
	OPDI_Port *first_port;
	OPDI_Port *last_port;

	OPDI_PortGroup *first_portGroup;
	OPDI_PortGroup *last_portGroup;

	uint32_t idle_timeout_ms;
	uint64_t last_activity;

	// internal shutdown function; to be called when messages can be sent to the master
	// May return OPDI_STATUS_OK to cancel the shutdown. Any other value stops message processing.
	uint8_t shutdownInternal(void);

public:
	/** Prepares the OPDI class for use.
	 * You can override this method to implement your platform specific setup.
	 */
	virtual uint8_t setup(const char *slave_name, int idleTimeout);

	/** Sets the idle timeout. If the master sends no meaningful messages during this time
	 * the slave sends a disconnect message. If the value is 0 (default), the setting has no effect.
	 * This functionality depends on the method getTimeMs to return meaningful values.
	 */
	virtual void setIdleTimeout(uint32_t idleTimeoutMs);

	/** Sets the encoding which must be a valid Java Charset identifier. Examples: ascii, utf-8, iso8859-1.
	 * The default encoding of this implementation is utf-8.
	 */
	virtual void setEncoding(const char* encoding);

	/** Adds the specified port.
	 * */
	virtual uint8_t addPort(OPDI_Port *port);

	/** Updates the internal port data structure. Is automatically called; do not use.
	*/
	virtual void updatePortData(OPDI_Port *port);

	/** Updates the internal port group data structure. Is automatically called; do not use.
	*/
	virtual void updatePortGroupData(OPDI_PortGroup *group);

	/** Internal function.
	 */
	virtual OPDI_Port *findPort(opdi_Port *port);

	/** Returns NULL if the port could not be found. */
	virtual OPDI_Port *findPortByID(const char *portID);

	/** Iterates through all ports and calls their prepare() methods. */
	virtual void preparePorts(void);

	/** Adds the specified port group. */
	virtual void addPortGroup(OPDI_PortGroup *portGroup);

	/** Starts the OPDI handshake to accept commands from a master.
	 * Does not use a housekeeping function.
	 */
	virtual uint8_t start(void);

	/** This function is called while the OPDI slave is connected and waiting for messages.
	 * It is called about once every millisecond.
	 * You can override it to perform your own housekeeping in case you need to.
	 * If canSend is 1, the slave may send asynchronous messages to the master.
	 * Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	 * This will usually signal a device error to the master or cause the master to time out.
	 * Make sure to call the base method before you do your own work.
	 */
	virtual uint8_t waiting(uint8_t canSend);

	/** This function returns 1 if a master is currently connected and 0 otherwise.
	 */
	virtual uint8_t isConnected(void);

	/** Sends the Disconnect message to the master and stops message processing.
	 */
	virtual uint8_t disconnect(void);

	/** Causes the Reconfigure message to be sent which prompts the master to re-read the device capabilities.
	 */
	virtual uint8_t reconfigure(void);

	/** Causes the Refresh message to be sent for the specified ports. The last element must be NULL.
	 *  If the first element is NULL, sends the empty refresh message causing all ports to be
	 *  refreshed.
	 */
	virtual uint8_t refresh(OPDI_Port **ports);

	/** This method is called when the idle timeout is reached. The default implementation sends a message
	 *  to the master and disconnects by returning OPDI_DISCONNECT. The method may return OPDI_STATUS_OK to stay connected.
	 */
	virtual uint8_t idleTimeoutReached(void);

	/** An internal handler which is used to implement the idle timer.
	 */
	virtual uint8_t messageHandled(channel_t channel, const char **parts);

	/** Disconnects a master if connected and releases resources. Frees all ports and stops message processing. */
	virtual void shutdown(void);

};

#endif
