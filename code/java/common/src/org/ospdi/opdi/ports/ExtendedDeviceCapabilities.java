package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.protocol.BasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.ExtendedProtocol;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;

/** DeviceCapabilities for a device that supports the Extended Protocol.
 * 
 * @author leo.meyer
 *
 */
public class ExtendedDeviceCapabilities extends BasicDeviceCapabilities {

	private ExtendedProtocol protocol;

	public ExtendedDeviceCapabilities(ExtendedProtocol protocol, int channel, String serialForm)
			throws ProtocolException, TimeoutException, InterruptedException,
			DisconnectedException, DeviceException {
		
		super((BasicProtocol)protocol, channel, serialForm);
		this.protocol = protocol;
	}
	
	@Override
	protected void addPorts(BasicProtocol protocol, int channel,
			String[] portIDs) throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException {
		
		this.addPorts((ExtendedProtocol)protocol, channel, portIDs);
	}

	protected void addPorts(ExtendedProtocol protocol, int channel,
			String[] portIDs) throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException {

		this.ports.addAll(protocol.getAllPortInfos(channel, portIDs));
	}
	
	@Override
	public void getPortStates() throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException,
			PortAccessDeniedException {
		
		protocol.getAllPortStates(this.ports);
	}
}
