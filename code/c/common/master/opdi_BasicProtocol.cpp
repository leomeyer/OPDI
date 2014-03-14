
#include "opdi_BasicProtocol.h"

#include "opdi_constants.h"
#include "opdi_protocol_constants.h"

#include <master\opdi_AbstractProtocol.h>
#include <master\opdi_StringTools.h>
#include <Poco\Thread.h>
#include <master\opdi_PortFactory.h>

PingRunner::PingRunner(IDevice* device)
{
	this->device = device;
	this->stopped = false;
}

void PingRunner::run()
{
	// send ping message on the control channel (channel == 0)
	Message ping = Message(0, PING_MESSAGE);
	while (!stopped && device->isConnected()) {
		try {
			// wait for the specified time after the last message
			while (GetTickCount() - device->getLastSendTimeMS() < PING_INTERVAL_MS)
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

bool BasicProtocol::dispatch(Message* message)
{
	// analyze control channel messages
	if (message->getChannel() == 0) {
		// received a disconnect?
		if (message->getPayload() == Disconnect) {
			// disconnect the device (this sends a disconnect message back)
			device->disconnect(false);
			return true;
		}
		else
		if (message->getPayload() == Reconfigure) {
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
			if (parts[0] == Debug) {
				device->receivedDebug(StringTools::join(1, 0, SEPARATOR, parts));
				return true;
			} else
			if (parts[0] == Refresh) {
				// remaining components are port IDs
				std::vector<std::string> portIDs;
				for (std::vector<std::string>::iterator iter = parts.begin() + 1; iter != parts.end(); iter++)
					portIDs.push_back(*iter);
				device->receivedRefresh(portIDs);
				return true;
			} else
			if (parts[0] == Error) {
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
	send(new Message(channel, getDeviceCaps));
	Message* capResult = expect(channel, DEFAULT_TIMEOUT);
		
	// decode the serial form
	// this may issue callbacks on the protocol which do not have to be threaded
	deviceCaps = new BasicDeviceCapabilities(this, channel, capResult->getPayload());

	return deviceCaps;
}

Port* BasicProtocol::findPortByID(std::string portID)
{
	return NULL;
}

Port* BasicProtocol::getPortInfo_(std::string id, int channel)
{
	send(new Message(channel, StringTools::join(AbstractProtocol::SEPARATOR, getPortInfo, id)));
	Message* message = expect(channel, DEFAULT_TIMEOUT);
		
	// check the port magic
	std::vector<std::string> parts;
	StringTools::split(message->getPayload(), SEPARATOR, parts);

	// let the factory create the port
	return PortFactory::createPort(*this, parts);
}

	/** Sets the mode for the given digital port and returns the new mode.
	 * 
	 * @param digitalPort
	 * @param mode
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortMode(DigitalPort digitalPort, DigitalPort.PortMode mode);

	/** Sets the line state for the given digital port and returns the new value.
	 * 
	 * @param digitalPort
	 * @param line
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortLine(DigitalPort digitalPort, DigitalPort.PortLine line) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;;
	


	void BasicProtocol::expectDigitalPortState(DigitalPort* port, int channel)
	{
		Message* m = expect(channel, DEFAULT_TIMEOUT);
		
		int PREFIX = 0;
		int ID = 1;
		int MODE = 2;
		int LINE = 3;
		int PARTS_COUNT = 4;
		
		std::vector<std::string> parts;
		StringTools::split(m->getPayload(), SEPARATOR, parts);
		if (parts.size() != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (parts[PREFIX] != digitalPortState)
			throw new ProtocolException(std::string("unexpected reply, expected: ") + digitalPortState);
		if (parts[ID] != port->getID())
			throw new ProtocolException("wrong port ID");

		// set port state
		port->setPortState(*this, (DigitalPortMode)AbstractProtocol::parseInt(parts[MODE], "mode", 0, 2));
		
		port->setPortLine(*this, 
				(DigitalPortLine)AbstractProtocol::parseInt(parts[LINE], "line", 0, 1));
	}

	void BasicProtocol::getPortState(DigitalPort* aDigitalPort)
	{
		expectDigitalPortState(aDigitalPort, send(new Message(getSynchronousChannel(), StringTools::join(SEPARATOR, getDigitalPortState, aDigitalPort->getID()))));		
	}

	/** Gets the state of an analog port. Returns the current value.
	 *  
	 * @param analogPort
	 * @return 
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void getPortState(AnalogPort analogPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;;

	/** Sets the mode for the given analog port and returns the new mode.
	 * 
	 * @param analogPort
	 * @param mode
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortMode(AnalogPort analogPort, AnalogPort.PortMode mode) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;;

	
	/** Sets the value of an analog port and returns the new value.
	 * 
	 * @param analogPort
	 * @param value
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortValue(AnalogPort analogPort, int value) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;;

	/** Sets the resolution of an analog port and returns the new value.
	 * 
	 * @param analogPort
	 * @param resolution
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortResolution(AnalogPort analogPort, AnalogPort.Resolution resolution) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Sets the reference of an analog port and returns the new value.
	 * 
	 * @param analogPort
	 * @param reference
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
//	void setPortReference(AnalogPort analogPort, AnalogPort.Reference reference) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Retrieves the label of the given position from a select port.
	 * 
	 * @param selectPort
	 * @param pos
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	String getLabel(SelectPort selectPort, int pos) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;
	
	/** Retrieves the current position setting of a select port as a zero-based integer value.
	 * 
	 * @param selectPort
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	void getPosition(SelectPort selectPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Sets the current position setting of a select port to the given value.
	 * Returns the current setting.
	 * @param selectPort
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	void setPosition(SelectPort selectPort, int pos) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Retrieves the current position setting of a dial port.
	 * 
	 * @param port
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	void getPosition(DialPort port) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;
	
	/** Sets the current position setting of a dial port to the given value.
	 * Returns the current setting.
	 * @param port
	 * @param pos
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	void setPosition(DialPort port, int pos) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;
	
	/** Binds the specified streaming port to a streaming channel.
	 * Returns true if the binding attempt was successful.
	 * 
	 * @param streamingPort
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	boolean bindStreamingPort(StreamingPort streamingPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Unbinds the specified streaming port.
	 * 
	 * @param streamingPort
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
//	void unbindStreamingPort(StreamingPort streamingPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;
	
	/** Sends the given data to the specified streaming port. Does not perform checks on the supplied data.
	 * 
	 * @param streamingPort
	 * @param data
	 * @throws DisconnectedException
	 */
//	void sendStreamingData(StreamingPort streamingPort, String data) throws DisconnectedException;
