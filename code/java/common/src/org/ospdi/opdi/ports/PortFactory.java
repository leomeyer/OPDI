package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.ProtocolException;


/** This class serves as a factory for basic port objects.
 * 
 * @author Leo
 *
 */
public class PortFactory {
	
	public static Port createPort(IBasicProtocol protocol, String[] parts) throws ProtocolException, TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		// the "magic" in the first part decides about the port type		
		if (parts[0].equals(DigitalPort.MAGIC))
			return new DigitalPort(protocol, parts);
		else if (parts[0].equals(AnalogPort.MAGIC))
			return new AnalogPort(protocol, parts);
		else if (parts[0].equals(SelectPort.MAGIC))
			return new SelectPort(protocol, parts);
		else if (parts[0].equals(DialPort.MAGIC))
			return new DialPort(protocol, parts);
		else if (parts[0].equals(StreamingPort.MAGIC))
			return new StreamingPort(protocol, parts);
		else
			throw new IllegalArgumentException("Programmer error: Unknown port magic: '" + parts[0] + "'. Protocol must check supported ports");
	}
}
