package org.ospdi.opdi.interfaces;

import java.util.List;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.AnalogPort;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.SelectPort;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.Message;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;


/** This interface specifies the basic protocol that must be supported by all devices.
 * 
 * @author Leo
 *
 */
public interface IBasicProtocol {
	
	/** Initiates the protocol. This may include starting a ping thread.
	 * 
	 */
	public void initiate();

	/** Disconnects the device in a regular way.
	 * 
	 */
	public void disconnect();

	/** Returns the protocol identifier.
	 * 
	 * @return
	 */
	public String getMagic();

	/** Attempts to dispatch a streaming message to a bound streaming port.
	 * If the message was dispatched, returns true, otherwise false.
	 * @param message
	 * @return
	 */
	public boolean dispatch(Message message);

	/** Returns the device capabilities.
	 * 
	 * @return
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws DeviceException 
	 * @throws ProtocolException 
	 * @throws TimeoutException 
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public IDeviceCapabilities getDeviceCapabilities() throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortMode(DigitalPort digitalPort, DigitalPort.PortMode mode) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortLine(DigitalPort digitalPort, DigitalPort.PortLine line) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;
	
	/** Gets the state for the given port.
	 * 
	 * @param digitalPort
	 * @return the channel number used for the query
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public int getPortState(DigitalPort digitalPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;

	/** Gets the state of an analog port. Returns the current value.
	 * 
	 * @param analogPort
	 * @return the channel number used for the query
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public int getPortState(AnalogPort analogPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortMode(AnalogPort analogPort, AnalogPort.PortMode mode) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;

	
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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortValue(AnalogPort analogPort, int value) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortResolution(AnalogPort analogPort, AnalogPort.Resolution resolution) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 * @throws AbortedException
	 */
	public void setPortReference(AnalogPort analogPort, AnalogPort.Reference reference) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public String getLabel(SelectPort selectPort, int pos) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

	/** Returns the labels for the specified select port.
	 * 
	 * @param selectPort
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 */
	public List<String> getSelectPortLabels(SelectPort selectPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Retrieves the current position setting of a select port as a zero-based integer value.
	 * 
	 * @param selectPort
	 * @return the channel number used for the query
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public int getPosition(SelectPort selectPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

	/** Sets the current position setting of a select port to the given value.
	 * Returns the current setting.
	 * @param selectPort
	 * @return
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public void setPosition(SelectPort selectPort, int pos) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

	/** Retrieves the current position setting of a dial port.
	 * 
	 * @param port
	 * @return the channel number used for the query
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public int getPosition(DialPort port) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException;
	
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
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public void setPosition(DialPort port, long pos) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException;
	
	/** Binds the specified streaming port to a streaming channel.
	 * Returns true if the binding attempt was successful.
	 * 
	 * @param streamingPort
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public boolean bindStreamingPort(StreamingPort streamingPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;

	/** Unbinds the specified streaming port.
	 * 
	 * @param streamingPort
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public void unbindStreamingPort(StreamingPort streamingPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException;
	
	/** Sends the given data to the specified streaming port. Does not perform checks on the supplied data.
	 * 
	 * @param streamingPort
	 * @param data
	 * @throws DisconnectedException
	 */
	public void sendStreamingData(StreamingPort streamingPort, String data) throws DisconnectedException;


}
