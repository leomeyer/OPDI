#ifndef __OPDI_BASICPROTOCOL_H
#define __OPDI_BASICPROTOCOL_H

#include <string>

#include "Poco/Exception.h"

#include "opdi_platformtypes.h"

#include "opdi_OPDIMessage.h"
#include "opdi_OPDIPort.h"
#include "opdi_BasicDeviceCapabilities.h"
#include "opdi_DigitalPort.h"

/** This interface specifies the basic protocol that must be supported by all devices.
 * 
 * @author Leo
 *
 */
class IBasicProtocol {
	
public:
	/** Initiates the protocol. This may include starting a ping thread.
	 * 
	 */
	virtual void initiate() = 0;

	/** Disconnects the device in a regular way.
	 * 
	 */
	virtual void disconnect() = 0;

	/** Returns the protocol identifier.
	 * 
	 * @return
	 */
	virtual std::string getMagic() = 0;

	/** Attempts to dispatch a streaming message to a bound streaming port.
	 * If the message was dispatched, returns true, otherwise false.
	 * @param message
	 * @return
	 */
	virtual bool dispatch(OPDIMessage* message) = 0;

	/** Returns the device capabilities.
	 * 
	 * @return
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws DeviceException 
	 * @throws ProtocolException 
	 * @throws TimeoutException 
	 */
	virtual BasicDeviceCapabilities* getDeviceCapabilities() = 0;
	
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
	virtual OPDIPort* findPortByID(std::string portID) = 0;
	
	/** Returns the information about the port with the given ID.
	 * Requires the channel from the initiating protocol.
	 * 
	 * @return
	 */
	virtual OPDIPort* getPortInfo(std::string id, int channel) = 0;

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
	virtual void setPortMode(DigitalPort* digitalPort, DigitalPortMode mode) = 0;

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
	virtual void setPortLine(DigitalPort* digitalPort, DigitalPortLine line) = 0;
	
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
	virtual void getPortState(DigitalPort* aDigitalPort) = 0;

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
