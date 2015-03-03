#ifndef __BASICPROTOCOL_H
#define __BASICPROTOCOL_H

#include "Poco/Runnable.h"
#include "Poco/Thread.h"

#include "opdi_IBasicProtocol.h"
#include "opdi_IDevice.h"
#include "opdi_AbstractProtocol.h"

#define DEFAULT_TIMEOUT				10000

#define PING_INTERVAL_MS			5000
#define	PING_MESSAGE				"ping"

#define CHANNEL_LOWEST_STREAMING	1
#define CHANNEL_HIGHEST_STREAMING	19
#define CHANNEL_LOWEST_SYNCHRONOUS	(CHANNEL_HIGHEST_STREAMING + 1)
#define CHANNEL_ROLLOVER			100

class PingRunner : public Poco::Runnable
{
private:
	IDevice* device;
	bool stopped;

public:
	PingRunner(IDevice* device);

	void run() override;

	void stop();
};

/** This class implements the basic protocol that must be supported by all devices.
 * 
 * @author Leo
 *
 */
class BasicProtocol : public AbstractProtocol {

private:
	PingRunner* pingRunner;
	Poco::Thread* pingThread;

	int currentChannel;

	BasicDeviceCapabilities* deviceCaps;

protected:

	void expectDigitalPortState(DigitalPort* port, int channel);

public:

	BasicProtocol(IDevice* device);
	
	/** Initiates the protocol. This may include starting a ping thread.
	 * 
	 */
	void initiate() override;

	/** Disconnects the device in a regular way.
	 * 
	 */
	void disconnect() override;

	/** Returns the protocol identifier.
	 * 
	 * @return
	 */
	std::string getMagic() override;

	/** Returns a new unique channel for a new protocol.
	 * 
	 * @return
	 */
	int getSynchronousChannel();

	/** Attempts to dispatch a streaming message to a bound streaming port.
	 * If the message was dispatched, returns true, otherwise false.
	 * @param message
	 * @return
	 */
	bool dispatch(OPDIMessage* message) override;

	/** Returns the device capabilities.
	 * 
	 * @return
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws DeviceException 
	 * @throws ProtocolException 
	 * @throws TimeoutException 
	 */
	BasicDeviceCapabilities* getDeviceCapabilities() override;
	
	/** Returns the port with the given ID. null if the port is not there.
	 * 
	 * @param portID
	 * @return
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws DeviceException 
	 * @throws ProtocolException 
	 * @throws TimeoutException 
	 */
	OPDIPort* findPortByID(std::string portID) override;
	
	/** Returns the information about the port with the given ID.
	 * Requires the channel from the initiating protocol.
	 * 
	 * @return
	 */
	OPDIPort* getPortInfo(std::string id, int channel) override;

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
	void setPortMode(DigitalPort* digitalPort, DigitalPortMode mode) override;

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
	void setPortLine(DigitalPort* digitalPort, DigitalPortLine line) override;


	/** Gets the state for the given port.
	 * 
	 * @param digitalPort
	 * @return 
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws AbortedException
	 */
	void getPortState(DigitalPort* aDigitalPort) override;

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

};

#endif
