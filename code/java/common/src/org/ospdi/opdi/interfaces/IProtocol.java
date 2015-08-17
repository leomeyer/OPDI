package org.ospdi.opdi.interfaces;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.Message;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.protocol.AbstractProtocol.IAbortable;

public interface IProtocol {

	enum ExpectationMode {
		NORMAL,
		IGNORE_REFRESHES
	};
	
	/** Returns the port with the given ID. null if the port is not there.
	 * 
	 * @param portID
	 * @return
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws DeviceException 
	 * @throws ProtocolException 
	 * @throws TimeoutException 
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public Port findPortByID(String portID) throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException, PortAccessDeniedException, PortErrorException;
	
	/** Returns the information about the port with the given ID.
	 * Requires the channel from the initiating protocol.
	 * 
	 * @return
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public Port getPortInfo(String id, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;
	

	public Message expect(long channel, int timeout, IAbortable abortable, ExpectationMode mode) 
			throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, PortAccessDeniedException, PortErrorException;

	public IDeviceCapabilities getDeviceCapabilities() throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException;

	public boolean dispatch(Message msg);

	public void initiate();

	public void disconnect();

	public String getMagic();
}
