
#include "Poco/Thread.h"

#include "opdi_constants.h"
#include "opdi_port.h"
#include "opdi_protocol_constants.h"

#include "opdi_platformfuncs.h"

#include "opdi_AbstractProtocol.h"
#include "opdi_BasicProtocol.h"
#include "opdi_StringTools.h"
#include "opdi_PortFactory.h"

PingRunner::PingRunner(IDevice* device)
{
	this->device = device;
	this->stopped = false;
}

void PingRunner::run()
{
	// send ping message on the control channel (channel == 0)
	OPDIMessage ping = OPDIMessage(0, PING_MESSAGE);
	while (!stopped && device->isConnected()) {
		try {
			// wait for the specified time after the last message
			while (opdi_get_time_ms() - device->getLastSendTimeMS() < PING_INTERVAL_MS)
				Poco::Thread::sleep(10);
			device->sendMessage(&ping);
			// sleep at least as long as specified by the interval
			Poco::Thread::sleep(PING_INTERVAL_MS);
		} catch (DisconnectedException e) {
			// the device has disconnected
			return;
		} catch (std::exception) {
			// ignore unexpected exception
			return;
		}
	}
}

void PingRunner::stop()
{
	stopped = true;
}

BasicProtocol::BasicProtocol(IDevice* device) : AbstractProtocol(device)
{
	pingRunner = NULL;
	currentChannel = CHANNEL_LOWEST_SYNCHRONOUS -1;
	deviceCaps = NULL;
}

void BasicProtocol::initiate()
{
	/*
		synchronized (boundStreamingPorts) {
			// clear streaming port bindings
			for (StreamingPort sp: boundStreamingPorts.values()) {
				sp.setChannel(this, 0);
			}
			// remove bindings
			boundStreamingPorts.clear();
		}
		// synchronous channel number reset
		currentChannel = CHANNEL_LOWEST_SYNCHRONOUS - 1;
	*/

	pingRunner = new PingRunner(device);

		// Start the ping thread
	pingThread = new Poco::Thread();
	pingThread->start(*pingRunner);
}

void BasicProtocol::disconnect()
{
	AbstractProtocol::disconnect();

	// stop ping thread
	if (pingRunner)
		pingRunner->stop();
}

std::string BasicProtocol::getMagic()
{
	return "BP";
}

/** Returns a new unique channel for a new protocol.
* 
* @return
*/
int BasicProtocol::getSynchronousChannel() {
	// calculate new unique channel number for synchronous protocol run
	int channel = currentChannel + 1;
	// prevent channel numbers from becoming too large
	if (channel >= CHANNEL_ROLLOVER)
		channel = 1;
	currentChannel = channel;
	return channel;
}

bool BasicProtocol::dispatch(OPDIMessage* message)
{
	// analyze control channel messages
	if (message->getChannel() == 0) {
		// received a disconnect?
		if (message->getPayload() == OPDI_Disconnect) {
			// disconnect the device (this sends a disconnect message back)
			device->disconnect(false);
			return true;
		}
		else
		if (message->getPayload() == OPDI_Reconfigure) {
			// clear cached device capabilities
//			deviceCaps = null;
			/*
			// reset bound streaming ports (reconfigure on the device unbinds streaming ports)
			synchronized (boundStreamingPorts) {
				for (StreamingPort sPort: boundStreamingPorts.values()) {
					sPort.portUnbound(this);
				}
				boundStreamingPorts.clear();
			}
			*/
			device->receivedReconfigure();
			return true;
		}
		else {
			// split message parts
			std::vector<std::string> parts;
			StringTools::split(message->getPayload(), SEPARATOR, parts);
				
			// received a debug message?
			if (parts[0] == OPDI_Debug) {
				device->receivedDebug(StringTools::join(1, 0, SEPARATOR, parts));
				return true;
			} else
			if (parts[0] == OPDI_Refresh) {
				// remaining components are port IDs
				std::vector<std::string> portIDs;
				for (std::vector<std::string>::iterator iter = parts.begin() + 1; iter != parts.end(); iter++)
					portIDs.push_back(*iter);
				device->receivedRefresh(portIDs);
				return true;
			} else
			if (parts[0] == OPDI_Error) {
				// remaining optional components contain the error information
				int error = 0;
				std::string text;
				if (parts.size() > 1) {
					error = parseInt(parts[1], "errorNo", 0, 255);
				}
				if (parts.size() > 2) {
					text = StringTools::join(2, 0, SEPARATOR, parts);
				}
				device->setError(error, text);
				return true;
			}
		}
	}
/*		
	synchronized (boundStreamingPorts) {
		// check whether the channel is bound to a streaming port
		StreamingPort sp = boundStreamingPorts.get(message->getChannel());
		if (sp != null) {
			// channel is bound; receive data
			sp.dataReceived(this, message->getPayload());
			return true;
		}
	}
*/			
	return false;
}

BasicDeviceCapabilities* BasicProtocol::getDeviceCapabilities()
{
	// has cached device capabilities?
	if (deviceCaps != NULL)
		return deviceCaps;

	int channel = getSynchronousChannel();
		
	// request device capabilities from the slave
	send(new OPDIMessage(channel, OPDI_getDeviceCaps));
	OPDIMessage* capResult = expect(channel, DEFAULT_TIMEOUT);
		
	// decode the serial form
	// this may issue callbacks on the protocol which do not have to be threaded
	deviceCaps = new BasicDeviceCapabilities(this, channel, capResult->getPayload());

	return deviceCaps;
}

OPDIPort* BasicProtocol::findPortByID(std::string portID)
{
	return NULL;
}

OPDIPort* BasicProtocol::getPortInfo(std::string id, int channel)
{
	send(new OPDIMessage(channel, StringTools::join(AbstractProtocol::SEPARATOR, OPDI_getPortInfo, id)));
	OPDIMessage* message = expect(channel, DEFAULT_TIMEOUT);
		
	// check the port magic
	std::vector<std::string> parts;
	StringTools::split(message->getPayload(), SEPARATOR, parts);

	// let the factory create the port
	return PortFactory::createPort(*this, parts);
}

void BasicProtocol::expectDigitalPortState(DigitalPort* port, int channel)
{
	OPDIMessage* m = expect(channel, DEFAULT_TIMEOUT);
		
	int PREFIX = 0;
	int ID = 1;
	int MODE = 2;
	int LINE = 3;
	unsigned int PART_COUNT = 4;
		
	std::vector<std::string> parts;
	StringTools::split(m->getPayload(), SEPARATOR, parts);
	if (parts.size() != PART_COUNT)
		throw ProtocolException("invalid number of message parts");
	if (parts[PREFIX] != OPDI_digitalPortState)
		throw ProtocolException(std::string("unexpected reply, expected: ") + OPDI_digitalPortState);
	if (parts[ID] != port->getID())
		throw ProtocolException("wrong port ID");

	// set port state
	port->setPortState(*this, (int8_t)AbstractProtocol::parseInt(parts[MODE], "mode", 0, 3));
		
	port->setPortLine(*this, 
			(int8_t)AbstractProtocol::parseInt(parts[LINE], "line", 0, 1));
}

void BasicProtocol::setPortMode(DigitalPort* digitalPort, int8_t mode)
{
	std::string portMode;
	switch (mode)
	{
	case OPDI_DIGITAL_MODE_INPUT_FLOATING: portMode = OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_FLOATING); break;
	case OPDI_DIGITAL_MODE_INPUT_PULLUP: portMode = OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_PULLUP); break;
	case OPDI_DIGITAL_MODE_INPUT_PULLDOWN: portMode = OPDI_QUOTE(OPDI_DIGITAL_MODE_INPUT_PULLDOWN); break;
	case OPDI_DIGITAL_MODE_OUTPUT: portMode = OPDI_QUOTE(OPDI_DIGITAL_MODE_OUTPUT); break;
	default: throw Poco::InvalidArgumentException("Invalid value for digital port mode");
	}

	expectDigitalPortState(digitalPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_setDigitalPortMode, digitalPort->getID(), portMode))));
}

void BasicProtocol::setPortLine(DigitalPort* digitalPort, int8_t line)
{
	std::string portLine;
	switch (line)
	{
	case OPDI_DIGITAL_LINE_LOW: portLine = OPDI_QUOTE(OPDI_DIGITAL_LINE_LOW); break;
	case OPDI_DIGITAL_LINE_HIGH: portLine = OPDI_QUOTE(OPDI_DIGITAL_LINE_HIGH); break;
	default: throw Poco::InvalidArgumentException("Invalid value for digital port line");
	}

	expectDigitalPortState(digitalPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_setDigitalPortLine, digitalPort->getID(), portLine))));
}

void BasicProtocol::getPortState(DigitalPort* aDigitalPort)
{
	expectDigitalPortState(aDigitalPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_getDigitalPortState, aDigitalPort->getID()))));		
}


void BasicProtocol::expectSelectPortPosition(SelectPort* port, int channel)
{
	OPDIMessage* m = expect(channel, DEFAULT_TIMEOUT);
		
	int PREFIX = 0;
	int ID = 1;
	int POSITION = 2;
	unsigned int PART_COUNT = 3;
		
	std::vector<std::string> parts;
	StringTools::split(m->getPayload(), SEPARATOR, parts);
	if (parts.size() != PART_COUNT)
		throw ProtocolException("invalid number of message parts");
	if (parts[PREFIX] != OPDI_selectPortState)
		throw ProtocolException(std::string("unexpected reply, expected: ") + OPDI_selectPortState);
	if (parts[ID] != port->getID())
		throw ProtocolException("wrong port ID");

	// set port state
	port->setPortPosition(*this, (uint16_t)AbstractProtocol::parseInt(parts[POSITION], "position", 0, port->getMaxPosition()));
}

std::string BasicProtocol::expectSelectPortLabel(SelectPort* port, int channel)
{
	OPDIMessage* m = expect(channel, DEFAULT_TIMEOUT);
		
	int PREFIX = 0;
	int ID = 1;
	int POSITION = 2;
	int LABEL = 3;
	unsigned int PART_COUNT = 4;
		
	std::vector<std::string> parts;
	StringTools::split(m->getPayload(), SEPARATOR, parts);
	if (parts.size() != PART_COUNT)
		throw ProtocolException("invalid number of message parts");
	if (parts[PREFIX] != OPDI_selectPortLabel)
		throw ProtocolException(std::string("unexpected reply, expected: ") + OPDI_digitalPortState);
	if (parts[ID] != port->getID())
		throw ProtocolException("wrong port ID");

	return parts[LABEL];
}

std::string BasicProtocol::getLabel(SelectPort *selectPort, int pos) {

	return expectSelectPortLabel(selectPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_getSelectPortLabel, selectPort->getID(), to_string((int)pos)))));
}

void BasicProtocol::getPortState(SelectPort *selectPort) {
	expectSelectPortPosition(selectPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_getSelectPortState, selectPort->getID()))));
}

void BasicProtocol::setPosition(SelectPort *selectPort, uint16_t pos) {
	expectSelectPortPosition(selectPort, send(new OPDIMessage(getSynchronousChannel(), StringTools::join(SEPARATOR, OPDI_setSelectPortPosition, selectPort->getID(), to_string((int)pos)))));
}
