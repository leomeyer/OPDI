package org.ospdi.opdi.protocol;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.PortFactory;
import org.ospdi.opdi.utils.Strings;

public class ExtendedProtocol extends BasicProtocol {

	private static final String MAGIC = "EP";

	public static final String GET_EXTENDED_PORT_INFO = "gEPI";
	public static final String EXTENDED_PORT_INFO = "EPI";
	
	public ExtendedProtocol(IDevice device) {
		super(device);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	public String getMagic() {
		return MAGIC;
	}

	public static String getMAGIC() {
		return MAGIC;
	}

	protected void expectExtendedPortState(Port port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal a port error", e);
		}
		
		final int PREFIX = 0;
		final int PORT_ID = 1;
		final int INFO = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_PORT_INFO))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_PORT_INFO);
		if (!parts[PORT_ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		// info is optional
		if (parts.length > INFO)
			port.setExtendedPortInfo(parts[INFO]);
	}
	

	@Override
	public Port getPortInfo(String portID, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Port result = super.getPortInfo(portID, channel);
		
		if (result != null) {
			// send extended port info request on the same channel
			send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_INFO, portID)));
			expectExtendedPortState(result, channel);
		}

		return result;
	}
	
}
