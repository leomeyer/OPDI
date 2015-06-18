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
 
#ifdef windows
#include <winsock2.h>
#include <windows.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <algorithm>

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/NObserver.h"
#include "Poco/Exception.h"
#include "Poco/Thread.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/URI.h"

#include <iostream>

#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_protocol_constants.h"
#include "master.h"
#include "opdi_main_io.h"
#include "opdi_AbstractProtocol.h"
#include "opdi_IDevice.h"
#include "opdi_TCPIPDevice.h"
#include "opdi_SerialDevice.h"

using Poco::Net::SocketReactor;
using Poco::Net::SocketAcceptor;
using Poco::Net::ReadableNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::ServerSocket;
using Poco::Net::StreamSocket;
using Poco::NObserver;
using Poco::AutoPtr;
using Poco::Thread;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;


// global output stream
OUTPUT_TYPE output;

// debug flag (for verbose logging)
bool var_debug = false;

void show_device_info(IDevice* device)
{
	output << "Device " << device->getID() << " (" << device->getStatusText() << "): " << device->getLabel() << (device->getStatus() == DS_CONNECTED ? " (" + device->getDeviceName() + ")" : "") << std::endl;	
}

class DeviceListener : public IDeviceListener 
{
public:
	
	void connectionOpened(IDevice* aDevice) override
	{
		output << "Device " << aDevice->getID() << ": Connection opened to " << aDevice->getDeviceName() << std::endl;
	}
			
	void connectionInitiated(IDevice* aDevice) override
	{
		output << "Device " << aDevice->getID() << ": Connection initiated" << std::endl;
	}
			
	void connectionAborted(IDevice* aDevice) override
	{
		output << "Device " << aDevice->getID() << ": Connection aborted" << std::endl;
	}

	void connectionFailed(IDevice* aDevice, std::string message) override 
	{
		output << "Device " << aDevice->getID() << ": Connection failed: " << message << std::endl;
	}

	void connectionTerminating(IDevice* aDevice) override
	{
		output << "Device " << aDevice->getID() << ": Connection terminating" << std::endl;
	}
			
	void connectionClosed(IDevice* aDevice) override
	{
		output << "Device " << aDevice->getID() << ": Connection closed" << std::endl;
	}

	void connectionError(IDevice* aDevice, std::string message) override
	{
		output << "Device " << aDevice->getID() << ": Connection error" << (message != "" ? ": " + message : "") << std::endl;
	}
			
	bool getCredentials(IDevice* device, std::string *user, std::string *password, bool *save) override
	{
		output << "Authentication required for ";
		show_device_info(device);

		*user = getLine("User: ");
		*password = getLine("Password: ");

		*save = true;
/*
		user = new std::string();
		password = new std::string();
		save = false;
*/
		return true;
	}
			
	void receivedDebug(IDevice* device, std::string message) override
	{
		output << "Device " << device->getID() << " says: " << message << std::endl;
	}
			
	void receivedReconfigure(IDevice* device) override 
	{
		output << "Received Reconfigure for device " << device->getID() << std::endl;
	}
			
	void receivedRefresh(IDevice* device, std::vector<std::string> portIDs) override 
	{
		output << "Received Refresh for device " << device->getID() << std::endl;
	}
			
	void receivedError(IDevice* device, std::string text) override
	{
		output << "Error on device " << device->getID() << ": " << text << std::endl;
	}
};


std::string opdiMasterName = "WinOPDI Master";

// device list
typedef std::vector<IDevice*> DeviceList;
DeviceList devices;

void show_help() 
{
	output << "Interactive OPDI master commands:" << std::endl;
	output << "? - show help" << std::endl;
	output << "quit - exit" << std::endl;
	output << "create_device <id> <address> - create a device; address must start with opdi_tcp://" << std::endl;
	output << "list - show the list of created devices" << std::endl;
	output << "connect <id> - connect to the specified device" << std::endl;
	output << "disconnect <id> - disconnect from the specified device" << std::endl;
	output << "set [<variable>] [<value>] - display or set control variable values" << std::endl;
	output << "" << std::endl;
	output << "Get started: Run slave and try 'create_device 1 opdi_tcp://localhost:13110'" << std::endl;
	output << "Next, connect the device: 'connect 1'" << std::endl;
	output << "Query device capabilities: 'gDC 1'" << std::endl;
}

IDevice *create_device(const std::string id, const std::string address) 
{
	// parse address
	Poco::URI uri;
	try
	{
		uri = Poco::URI(address);
	}
	catch (Poco::Exception& e) 
	{
		throw Poco::InvalidArgumentException("Address parse error", e, 0);
	}
	
	if (strcmp(uri.getScheme().c_str(), "opdi_tcp") == 0) {

		return new TCPIPDevice(id, uri, &var_debug);
	} else
	if (strcmp(uri.getScheme().c_str(), "opdi_com") == 0) {

		return new SerialDevice(id, uri, &var_debug);
	} else {
		throw Poco::UnknownURISchemeException("Address schema not recognized", uri.getScheme(), 0);
		return NULL;
	}
}

void print_exception(const Poco::Exception* e, bool inner = false) 
{
	output << (inner ? "Caused by: " : "") << e->displayText() << std::endl;
	if (e->nested() != NULL)
		print_exception(e->nested(), true);
}

IDevice* find_device(std::string devID, bool throwIfNotFound)
{
	IDevice* result = NULL;

	// check whether it's already contained
	DeviceList::iterator iter;
	for (iter = devices.begin(); iter != devices.end(); ++iter)
	{
		if ((*iter)->getID() == devID)
			result = *iter;
	}

	if (result == NULL && throwIfNotFound)
	{
		throw Poco::InvalidArgumentException("Unknown device ID: " + devID);
	}

	return result;
}

void add_device(IDevice* device)
{
	// check whether it's already contained
	DeviceList::iterator iter;
	for (iter = devices.begin(); iter != devices.end(); ++iter)
	{
		if (*device == *iter)
			break;
	}

	// found it?
	if (iter != devices.end()) {
		output << "Cannot add device: A device with ID " << device->getID() << " already exists" << std::endl;
	} else {
		// add the device
		devices.push_back(device);
		output << "Device " << device->getID() << " added: " << device->getLabel() << std::endl;
	}
}

void list_devices()
{
	if (devices.size() == 0) {
		output << "No devices" << std::endl;
		return;
	}

	DeviceList::iterator iter;
	for (iter = devices.begin(); iter != devices.end(); ++iter)
	{
		IDevice* device = *iter;
		show_device_info(device);
	}
}

void show_variable(std::string variable)
{
	if (variable == "debug") {
		output << "debug " << var_debug << std::endl;
	} else
		output << "Unknown variable: " << variable << std::endl;
}

void show_variables()
{
	// show all variables
	show_variable("debug");
}

bool get_bool_value(std::string variable, std::string value, bool *var)
{
	if (value == "0")
		*var = false;
	else
	if (value == "1")
		*var = true;
	else {
		output << "Invalid value for boolean variable " << variable << "; try 0 or 1" << std::endl;
		return false;
	}
	return true;
}

void set_variable(std::string variable, std::string value)
{
	bool ok;
	if (variable == "debug") {
		ok = get_bool_value(variable, value, &var_debug);
	}
	if (ok)
		show_variable(variable);
}

void cleanup()
{
	// disconnect all connected devices
	DeviceList::iterator iter;
	for (iter = devices.begin(); iter != devices.end(); ++iter)
	{
		IDevice* device = *iter;
		if (device->isConnected())
			device->disconnect(false);
	}
}

void print_devicecaps(BasicDeviceCapabilities* bdc)
{
	for (std::vector<OPDIPort*>::iterator iter = bdc->getPorts().begin(); iter != bdc->getPorts().end(); iter++) {
		output << (*iter)->toString() << std::endl;
	}
}

DigitalPort *checkDigitalPort(std::string cmd, OPDIPort *port) {
	if (port->getType() != PORTTYPE_DIGITAL) {
		output << "Expected digital port for command: " << cmd << std::endl;
		return NULL;
	}
	return (DigitalPort *)port;
}

SelectPort *checkSelectPort(std::string cmd, OPDIPort *port) {
	if (port->getType() != PORTTYPE_SELECT) {
		output << "Expected select port for command: " << cmd << std::endl;
		return NULL;
	}
	return (SelectPort *)port;
}

bool digital_port_command(std::string cmd, OPDIPort* port)
{
	const char *part;

	if (cmd == OPDI_setDigitalPortMode) {
		part = strtok(NULL, " ");
		if (part == NULL) {
			output << "Error: Digital port mode expected" << std::endl;
			return false;
		}

		int8_t dpm = OPDI_DIGITAL_MODE_UNKNOWN;
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_FLOATING)) == 0)
			dpm = OPDI_DIGITAL_MODE_INPUT_FLOATING;
		else
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_PULLUP)) == 0)
			dpm = OPDI_DIGITAL_MODE_INPUT_PULLUP;
		else
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_PULLDOWN)) == 0)
			dpm = OPDI_DIGITAL_MODE_INPUT_PULLDOWN;
		else
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_MODE_OUTPUT)) == 0)
			dpm = OPDI_DIGITAL_MODE_OUTPUT;

		DigitalPort* thePort = checkDigitalPort(cmd, port);
		if (thePort == NULL)
			return false;
		thePort->setMode(dpm);
		return true;
	} else
	if (cmd == OPDI_setDigitalPortLine)	{
		part = strtok(NULL, " ");
		if (part == NULL) {
			output << "Error: Digital port line expected" << std::endl;
			return false;
		}

		int8_t dpl = OPDI_DIGITAL_LINE_UNKNOWN;
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_LINE_LOW)) == 0)
			dpl = OPDI_DIGITAL_LINE_LOW;
		else
		if (strcmp(part, OPDI_QUOTE(OPDI_DIGITAL_LINE_HIGH)) == 0)
			dpl = OPDI_DIGITAL_LINE_HIGH;

		DigitalPort* thePort = checkDigitalPort(cmd, port);
		if (thePort == NULL)
			return false;
		thePort->setLine(dpl);
		return true;
	} else 
	if (cmd == OPDI_getDigitalPortState)	{
		DigitalPort* thePort = checkDigitalPort(cmd, port);
		if (thePort == NULL)
			return false;
		thePort->getPortState();
		return true;
	}
	output << "Command not implemented: " << cmd << std::endl;
	return false;
}

bool select_port_command(std::string cmd, OPDIPort* port)
{
	const char *part;
	if (cmd == OPDI_getSelectPortState)	{
		SelectPort* thePort = checkSelectPort(cmd, port);
		if (thePort == NULL)
			return false;
		thePort->getPortState();
	} else 
	if (cmd == OPDI_setSelectPortPosition)	{
		part = strtok(NULL, " ");
		if (part == NULL) {
			output << "Error: Select port position expected" << std::endl;
			return false;
		}
		uint16_t pos = atoi(part);

		SelectPort* thePort = checkSelectPort(cmd, port);
		if (thePort == NULL)
			return false;
		thePort->setPosition(pos);
		return true;
	}
	output << "Command not implemented: " << cmd << std::endl;
	return false;
}

// commands that operate on ports
// find device, find port, execute command
bool port_command(const char *part)
{
	std::string cmd = part;

	// expect second token: device ID
	part = strtok(NULL, " ");
	if (part == NULL) {
		output << "Error: Device ID expected" << std::endl;
		return false;
	}
	std::string devID = part;

	// find device
	// throw exception if not found
	IDevice *device = find_device(devID, true);

	if (device->getStatus() != DS_CONNECTED) {
		output << "Device " << device->getID() << " is not connected" << std::endl;
		return false;
	}

	// expect third token: port ID
	part = strtok(NULL, " ");
	if (part == NULL) {
		output << "Error: Port ID expected" << std::endl;
		return false;
	}
	std::string portID = part;

	// find port
	OPDIPort* port = device->getCapabilities()->findPortByID(portID);

	if (port == NULL) {
		output << "Error: Port not found: " << portID << std::endl;
		return false;
	}

	// check command
	if (cmd == OPDI_setDigitalPortMode || cmd == OPDI_setDigitalPortLine || cmd == OPDI_getDigitalPortState)
	{
		if (digital_port_command(cmd, port))
		{
			output << port->toString() << std::endl;
			return true;
		}
	} else 
	if (cmd == OPDI_getSelectPortState || cmd == OPDI_setSelectPortPosition)
	{
		if (select_port_command(cmd, port))
		{
			output << port->toString() << std::endl;
			return true;
		}
	} else
	{
		output << "Command not implemented: " << cmd << std::endl;
	}

	return false;
}

int start_master() 
{
	#define PROMPT	"$ "

	const char *part;

	printf("Interactive OPDI master started. Type '?' for help.\n");

	add_device(create_device("1", "opdi_tcp://admin:admin@localhost:13110"));
	add_device(create_device("2", "opdi_tcp://admin:admin@192.168.56.101"));

	std::string cmd;
	while (true) {
		cmd = getLine(PROMPT);

		// evaluate command
		try	{
			// tokenize the input
			part = strtok((char*)cmd.c_str(), " ");
			if (part == NULL)
				continue;
			// evaluate command
			if (strcmp(part, "?") == 0) {
				show_help();
			} else
			if ((strcmp(part, "quit") == 0) || (strcmp(part, "exit") == 0)) {
				cleanup();
				break;
			} else
			if (strcmp(part, "list") == 0) {
				list_devices();
			} else
			if (strcmp(part, "create_device") == 0) {
				// expect second token: device ID
				part = strtok(NULL, " ");
				if (part == NULL) {
					output << "Error: Device ID expected" << std::endl;
					continue;
				}
				std::string devID = part;

				// expect third token: slave address
				part = strtok(NULL, " ");
				if (part == NULL) {
					output << "Error: Slave address expected" << std::endl;
					continue;
				}
				std::string address = part;

				// create the device
				IDevice* device = create_device(devID, address);

				add_device(device);
			} else
			if (strcmp(part, "connect") == 0) {
				// expect second token: device ID
				part = strtok(NULL, " ");
				if (part == NULL) {
					output << "Error: Device ID expected" << std::endl;
					continue;
				}
				std::string devID = part;

				// throw exception if not found
				IDevice *device = find_device(devID, true);

				if (!device->prepare())
					continue;

				// prepare the device for connection
				if (device->getStatus() == DS_CONNECTED) {
					output << "Device " << device->getID() << " is already connected" << std::endl;
					continue;
				} else
				if (device->getStatus() == DS_CONNECTING) {
					output << "Device " << device->getID() << " is already connecting" << std::endl;
					continue;
				} else {
					device->connect(new DeviceListener());
				}
			} else 
			if (strcmp(part, OPDI_getDeviceCaps) == 0) {
				// expect second token: device ID
				part = strtok(NULL, " ");
				if (part == NULL) {
					output << "Error: Device ID expected" << std::endl;
					continue;
				}
				std::string devID = part;

				// throw exception if not found
				IDevice *device = find_device(devID, true);

				if (device->getStatus() != DS_CONNECTED) {
					output << "Device " << device->getID() << " is not connected" << std::endl;
					continue;
				}

				// query device capabilities
				print_devicecaps(device->getCapabilities());
			} else 
			if (strcmp(part, OPDI_setDigitalPortMode) == 0) {
				port_command(part);
			} else 
			if (strcmp(part, OPDI_setDigitalPortLine) == 0) {
				port_command(part);
			} else 
			if (strcmp(part, OPDI_getDigitalPortState) == 0) {
				port_command(part);
			} else 
			if (strcmp(part, OPDI_getSelectPortState) == 0) {
				port_command(part);
			} else 
			if (strcmp(part, OPDI_setSelectPortPosition) == 0) {
				port_command(part);
			} else 
			if (strcmp(part, "disconnect") == 0) {
				// expect second token: device ID
				part = strtok(NULL, " ");
				if (part == NULL) {
					output << "Error: Device ID expected" << std::endl;
					continue;
				}
				std::string devID = part;

				// throw exception if not found
				IDevice *device = find_device(devID, true);

				if (device->getStatus() == DS_DISCONNECTED) {
					output << "Device " << device->getID() << " is already disconnected" << std::endl;
					continue;
				}
				// disconnect the device
				device->disconnect(false);
			} else 
			if (strcmp(part, "set") == 0) {
				// expect second token: variable
				part = strtok(NULL, " ");
				if (part == NULL) {
					show_variables();
					continue;
				}
				std::string variable = part;

				// optional third token: variable value
				part = strtok(NULL, " ");
				if (part == NULL) {
					show_variable(variable);
					continue;
				}

				// set the value
				set_variable(variable, part);
			} else 
			{
				output << "Command unknown" << std::endl;
			}
		}
		catch (const ProtocolException& pe) {
			output << "Protocol exception: ";
			print_exception(&pe);
		}
		catch (const Poco::Exception& e) {
			print_exception(&e);
		}
		catch (const std::exception&) {
			output << "Unknown exception\n";
		}
		catch (...) {
			output << "Unknown error\n";
		}
	}		// while

	return 0;
}
