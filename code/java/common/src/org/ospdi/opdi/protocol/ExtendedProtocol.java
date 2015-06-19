package org.ospdi.opdi.protocol;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.devices.DeviceInfo;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.ports.BasicDeviceCapabilities;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.PortGroup;
import org.ospdi.opdi.utils.Strings;

public class ExtendedProtocol extends BasicProtocol {

	private static final String MAGIC = "EP";

	public static final String GET_EXTENDED_PORT_INFO = "gEPI";
	public static final String EXTENDED_PORT_INFO = "EPI";

	public static final String GET_GROUP_INFO = "gGI";
	public static final String GROUP_INFO = "GI";
	public static final String GET_EXTENDED_GROUP_INFO = "gEGI";
	public static final String EXTENDED_GROUP_INFO = "EGI";

	public static final String GET_EXTENDED_DEVICE_INFO = "gEDI";
	public static final String EXTENDED_DEVICE_INFO = "EDI";
	
	protected Map<String, PortGroup> groupCache = new HashMap<String, PortGroup>();
	
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
		
		// check whether there's a group specified
		String group = port.getExtendedProperty("group", ""); 
		if (!group.isEmpty()) {
			// assign group
			port.setGroup(this.getGroupInfo(group, channel));
		}
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
	
	protected void expectExtendedGroupInfo(Port port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
/**
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
*/			
	}
	
	public PortGroup getGroupInfo(String groupID, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		
		// check whether the group has already been cached
		if (groupCache.containsKey(groupID))
			return groupCache.get(groupID);
		
		send(new Message(channel, Strings.join(SEPARATOR, GET_GROUP_INFO, groupID)));
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getGroupInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getGroupInfo should never signal a port error", e);
		}
		
		final int PREFIX = 0;
		final int GROUP_ID = 1;
		final int LABEL = 2;
		final int PARENT = 3;
		final int FLAGS = 4;
		final int PARTS_COUNT = 5;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(GROUP_INFO))
			throw new ProtocolException("unexpected reply, expected: " + GROUP_INFO);
		if (!parts[GROUP_ID].equals(groupID))
			throw new ProtocolException("wrong group ID");
		
		PortGroup result = new PortGroup(groupID, parts[LABEL], parts[PARENT], Strings.parseInt(parts[FLAGS], "flags", 0, Integer.MAX_VALUE));
		groupCache.put(groupID, result);
		
		if (!result.getParent().isEmpty()) {
			
			// load parent group information
			PortGroup parent = getGroupInfo(result.getParent(), channel);
			
			// assign parent (cycle check)
			result.setParentGroup(parent);
		}
		
		return result;
	}

	
	protected DeviceInfo expectExtendedDeviceInfo(int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal a port error", e);
		}
		
		final int PREFIX = 0;
		final int INFO = 1;
		final int PARTS_COUNT = 2;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_DEVICE_INFO))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_DEVICE_INFO);
		// info is required
		if (parts.length <= INFO)
			throw new ProtocolException("invalid number of message parts");
		return new DeviceInfo(parts[INFO]);
			
	}

	public DeviceInfo getExtendedDeviceInfo() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		
		int channel = this.getSynchronousChannel();
		
		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_DEVICE_INFO)));
		return expectExtendedDeviceInfo(channel);
	}
	
	@Override
	public BasicDeviceCapabilities getDeviceCapabilities()
			throws TimeoutException, ProtocolException, DeviceException,
			InterruptedException, DisconnectedException {
		BasicDeviceCapabilities result = super.getDeviceCapabilities();
		
		// additional query: extended device info
		this.getDevice().setDeviceInfo(getExtendedDeviceInfo());
		
		return result;
	}

}
